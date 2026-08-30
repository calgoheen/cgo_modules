#include <cgo_graph/cgo_graph.h>

namespace cgo
{

namespace modulationIds
{

const juce::Identifier MODULATION { "MODULATION" };
const juce::Identifier M { "M" };
const juce::Identifier id { "id" };
const juce::Identifier src { "src" };
const juce::Identifier tgtKind { "tgtKind" };
const juce::Identifier tgt { "tgt" };
const juce::Identifier param { "param" };
const juce::Identifier depth { "depth" };
const juce::Identifier bipolar { "bipolar" };

const juce::String depthTarget { "depth" };

} // namespace modulationIds

ModulationGraph::ScopedBatch::ScopedBatch (ModulationGraph& g) : graph (g) { ++graph.batchDepth; }

ModulationGraph::ScopedBatch::~ScopedBatch()
{
    if (--graph.batchDepth > 0 || ! graph.compilePending)
        return;

    graph.compilePending = false;
    graph.compile();
}

ModulationGraph::ModulationGraph (const ProcessorGraph& processors, const ModulatorRegistry& modulators)
  : processorGraph (processors), modulatorRegistry (modulators)
{
}

std::optional<ConnectionID> ModulationGraph::addModulation (ModulatorID source, NodeRef targetNode, int targetParam, float initialDepth, bool bipolar)
{
    return addModulation (source, Target { ParamTarget { targetNode, targetParam } }, initialDepth, bipolar, std::nullopt);
}

std::optional<ConnectionID> ModulationGraph::addDepthModulation (ConnectionID connection, ModulatorID source, float initialDepth, bool bipolar)
{
    // No connection with this ID
    jassert (connections.find (connection) != connections.end());

    return addModulation (source, Target { DepthTarget { connection } }, initialDepth, bipolar, std::nullopt);
}

void ModulationGraph::removeModulation (ConnectionID id)
{
    // No connection with this ID
    jassert (connections.find (id) != connections.end());

    removeConnection (id);
    compile();
}

void ModulationGraph::setModulationDepth (ConnectionID id, float depth)
{
    const auto it = connections.find (id);

    // No connection with this ID
    jassert (it != connections.end());

    if (it != connections.end())
        it->second.depth->setValue (depth);
}

void ModulationGraph::setModulationBipolar (ConnectionID id, bool bipolar)
{
    const auto it = connections.find (id);

    // No connection with this ID
    jassert (it != connections.end());

    if (it == connections.end() || it->second.bipolar == bipolar)
        return;

    it->second.bipolar = bipolar;
    compile();
}

std::optional<ConnectionID> ModulationGraph::findModulation (ModulatorID source, NodeRef targetNode, int targetParam) const
{
    return findModulation (source, Target { ParamTarget { targetNode, targetParam } });
}

std::optional<ConnectionID> ModulationGraph::findDepthModulation (ModulatorID source, ConnectionID connection) const
{
    return findModulation (source, Target { DepthTarget { connection } });
}

std::vector<ModulationGraph::ModulationEntry> ModulationGraph::getModulations() const
{
    std::vector<ModulationEntry> entries;
    entries.reserve (connections.size());

    for (const auto& [id, connection] : connections)
        entries.push_back ({ id, connection.source, connection.target, connection.depth->getTargetValue(), connection.bipolar });

    return entries;
}

std::vector<ModulationGraph::ModulationEntry> ModulationGraph::getModulationsFor (NodeRef node, int param) const
{
    return getModulationsFor (Target { ParamTarget { node, param } });
}

std::vector<ModulationGraph::ModulationEntry> ModulationGraph::getDepthModulations (ConnectionID connection) const
{
    return getModulationsFor (Target { DepthTarget { connection } });
}

std::vector<ModulationGraph::ModulationEntry> ModulationGraph::getModulationsFrom (ModulatorID source) const
{
    std::vector<ModulationEntry> entries;

    for (const auto& [id, connection] : connections)
        if (connection.source == source)
            entries.push_back ({ id, connection.source, connection.target, connection.depth->getTargetValue(), connection.bipolar });

    return entries;
}

std::optional<ModulationGraph::ModulationEntry> ModulationGraph::getModulation (ConnectionID id) const
{
    const auto it = connections.find (id);

    if (it == connections.end())
        return std::nullopt;

    return ModulationEntry { id, it->second.source, it->second.target, it->second.depth->getTargetValue(), it->second.bipolar };
}

void ModulationGraph::restoreModulations (const std::vector<ModulationEntry>& entries)
{
    mintModulations (entries);
    compile();
}

void ModulationGraph::prepare (double sampleRate, int blockSize)
{
    cachedSampleRate = sampleRate;
    cachedBlockSize = blockSize;
    prepared = true;

    for (auto& [id, connection] : connections)
        connection.depth->prepare (sampleRate, blockSize);
}

void ModulationGraph::compile()
{
    JUCE_ASSERT_MESSAGE_THREAD

    if (batchDepth > 0)
    {
        compilePending = true;
        return;
    }

    auto snapshot = std::make_unique<ModulationSnapshot>();

    // 1. Group connections by target, so that the slots for one ModulatedValue end up
    //    contiguous and can be named by a single SlotRange.
    std::vector<Target> targets;
    std::vector<std::vector<ConnectionID>> grouped;

    for (const auto& [id, connection] : connections)
    {
        const auto it = std::find (targets.begin(), targets.end(), connection.target);
        const auto index = (size_t) std::distance (targets.begin(), it);

        if (it == targets.end())
        {
            targets.push_back (connection.target);
            grouped.emplace_back();
        }

        grouped[index].push_back (id);
    }

    std::vector<SlotRange> targetRanges;
    targetRanges.reserve (targets.size());

    int numSlots = 0;

    for (const auto& group : grouped)
    {
        targetRanges.push_back ({ numSlots, (int) group.size() });
        numSlots += (int) group.size();
    }

    auto rangeFor = [&] (const Target& target) -> SlotRange
    {
        const auto it = std::find (targets.begin(), targets.end(), target);
        return it != targets.end() ? targetRanges[(size_t) std::distance (targets.begin(), it)] : SlotRange {};
    };

    // 2. Emit the slots themselves. A slot names its depth's own modulation by range
    //    rather than by pointer, so the array can reference itself while being built.
    snapshot->slots.resize ((size_t) numSlots);

    for (size_t g = 0; g < grouped.size(); g++)
    {
        for (size_t i = 0; i < grouped[g].size(); i++)
        {
            const auto id = grouped[g][i];
            const auto& connection = connections.at (id);

            snapshot->slots[(size_t) targetRanges[g].begin + i] = { &resolveSource (connection.source),
                                                                    connection.depth,
                                                                    rangeFor (Target { DepthTarget { id } }),
                                                                    connection.bipolar };
        }
    }

    // 3. Topo-sort the modulators. A connection whose target resolves to a modulator is
    //    an edge source -> owner: the source must fill its buffer before the owner runs.
    const auto modulatorEntries = modulatorRegistry.getAll();
    const int numModulators = (int) modulatorEntries.size();

    auto indexOfModulator = [&] (ModulatorID id)
    {
        for (int i = 0; i < numModulators; i++)
            if (modulatorEntries[(size_t) i].id == id)
                return i;

        return -1;
    };

    std::vector<std::pair<int, int>> edges;

    for (const auto& [id, connection] : connections)
    {
        const auto owner = ownerNodeOf (id);

        if (! owner.has_value() || ! std::holds_alternative<ModulatorID> (*owner))
            continue;

        const int from = indexOfModulator (connection.source);
        const int to = indexOfModulator (std::get<ModulatorID> (*owner));

        if (from >= 0 && to >= 0 && from != to)
            edges.emplace_back (from, to);
    }

    std::sort (edges.begin(), edges.end());
    edges.erase (std::unique (edges.begin(), edges.end()), edges.end());

    std::vector<std::vector<int>> successors ((size_t) numModulators);
    std::vector<int> inDegree ((size_t) numModulators, 0);

    for (const auto& [from, to] : edges)
    {
        successors[(size_t) from].push_back (to);
        ++inDegree[(size_t) to];
    }

    std::vector<int> ready;
    std::vector<bool> emitted ((size_t) numModulators, false);

    for (int i = 0; i < numModulators; i++)
        if (inDegree[(size_t) i] == 0)
            ready.push_back (i);

    std::vector<int> order;
    order.reserve ((size_t) numModulators);

    auto emit = [&] (int i)
    {
        emitted[(size_t) i] = true;
        order.push_back (i);

        for (const int j : successors[(size_t) i])
            if (--inDegree[(size_t) j] == 0 && ! emitted[(size_t) j])
                ready.push_back (j);
    };

    // Whether `start` can reach itself over the nodes still waiting - i.e. whether it is on
    // a cycle rather than merely downstream of one.
    auto liesOnCycle = [&] (int start)
    {
        std::vector<bool> seen ((size_t) numModulators, false);
        std::vector<int> stack;

        for (const int j : successors[(size_t) start])
            if (! emitted[(size_t) j])
                stack.push_back (j);

        while (! stack.empty())
        {
            const int n = stack.back();
            stack.pop_back();

            if (n == start)
                return true;

            if (seen[(size_t) n])
                continue;

            seen[(size_t) n] = true;

            for (const int j : successors[(size_t) n])
                if (! emitted[(size_t) j])
                    stack.push_back (j);
        }

        return false;
    };

    while ((int) order.size() < numModulators)
    {
        while (! ready.empty())
        {
            const int i = ready.back();
            ready.pop_back();
            emit (i);
        }

        if ((int) order.size() == numModulators)
            break;

        int forced = -1;

        for (int i = 0; i < numModulators && forced < 0; i++)
            if (! emitted[(size_t) i] && liesOnCycle (i))
                forced = i;

        jassert (forced >= 0);

        for (int i = 0; i < numModulators && forced < 0; i++)
            if (! emitted[(size_t) i])
                forced = i;

        emit (forced);
    }

    // 4. Give every node its parameter ranges. paramRanges has to be complete before any
    //    NodeModulation points into it, so record the offsets now and fix up the views
    //    once both vectors have stopped growing.
    std::vector<std::pair<int, int>> nodeRanges;

    auto appendNode = [&] (const NodeRef& node, const ParameterOwner& owner)
    {
        const int numParams = owner.getModulatedParameters().size();
        const int base = (int) snapshot->paramRanges.size();

        for (int i = 0; i < numParams; i++)
            snapshot->paramRanges.push_back (rangeFor (Target { ParamTarget { node, i } }));

        nodeRanges.emplace_back (base, numParams);
    };

    for (const int i : order)
    {
        const auto& entry = modulatorEntries[(size_t) i];

        appendNode (NodeRef { entry.id }, *entry.modulator);
        snapshot->modulators.push_back ({ Modulator::Ptr (entry.modulator), {} });
    }

    for (const auto& node : processorGraph.getProcessorNodes())
    {
        appendNode (NodeRef { node->nodeID }, *static_cast<Processor*> (node->getProcessor()));
        snapshot->processors.push_back ({ node, {} });
    }

    const auto* slotBase = snapshot->slots.data();
    const auto* rangeBase = snapshot->paramRanges.data();
    const auto numModulatorEntries = snapshot->modulators.size();

    for (size_t i = 0; i < numModulatorEntries; i++)
    {
        const auto [base, numParams] = nodeRanges[i];
        snapshot->modulators[i].modulation = { slotBase, rangeBase + base, numParams };
    }

    for (size_t i = 0; i < snapshot->processors.size(); i++)
    {
        const auto [base, numParams] = nodeRanges[numModulatorEntries + i];
        snapshot->processors[i].modulation = { slotBase, rangeBase + base, numParams };
    }

    snapshotExchange.set (std::move (snapshot));
}

ModulationSnapshot* ModulationGraph::loadSnapshot() { return snapshotExchange.loadAudioThreadState(); }

juce::ValueTree ModulationGraph::toValueTree() const
{
    juce::ValueTree tree { modulationIds::MODULATION };

    for (const auto& [id, connection] : connections)
    {
        juce::ValueTree m { modulationIds::M };

        m.setProperty (modulationIds::id, (int) id.uid, nullptr);
        m.setProperty (modulationIds::src, (int) connection.source.uid, nullptr);

        if (const auto* param = std::get_if<ParamTarget> (&connection.target))
        {
            m.setProperty (modulationIds::tgtKind, std::holds_alternative<ProcessorID> (param->node) ? NodeKind::processor : NodeKind::modulator, nullptr);
            m.setProperty (modulationIds::tgt, (int) std::visit ([] (auto node) { return node.uid; }, param->node), nullptr);
            m.setProperty (modulationIds::param, resolveNode (param->node).getModulatedParameters()[param->index]->parameter.getParameterID(), nullptr);
        }
        else
        {
            m.setProperty (modulationIds::tgtKind, modulationIds::depthTarget, nullptr);
            m.setProperty (modulationIds::tgt, (int) std::get<DepthTarget> (connection.target).connection.uid, nullptr);
        }

        m.setProperty (modulationIds::depth, connection.depth->getTargetValue(), nullptr);
        m.setProperty (modulationIds::bipolar, connection.bipolar, nullptr);

        tree.appendChild (m, nullptr);
    }

    return tree;
}

void ModulationGraph::restoreFromValueTree (const juce::ValueTree& graphTree)
{
    jassert (connections.empty());

    std::vector<ModulationEntry> entries;

    for (const auto m : graphTree.getChildWithName (modulationIds::MODULATION))
    {
        if (! m.hasType (modulationIds::M))
            continue;

        const ConnectionID id { (juce::uint32) (int) m[modulationIds::id] };
        const ModulatorID source { (juce::uint32) (int) m[modulationIds::src] };
        const auto kind = m[modulationIds::tgtKind].toString();
        const auto targetUid = (juce::uint32) (int) m[modulationIds::tgt];

        const auto target = [&]() -> std::optional<Target>
        {
            if (kind == modulationIds::depthTarget)
                return Target { DepthTarget { ConnectionID { targetUid } } };

            if (kind != NodeKind::processor && kind != NodeKind::modulator)
                return std::nullopt;

            const NodeRef node = kind == NodeKind::processor ? NodeRef { ProcessorID { targetUid } } : NodeRef { ModulatorID { targetUid } };

            if (! contains (node))
                return std::nullopt;

            const int index = resolveNode (node).indexOf (m[modulationIds::param].toString());

            if (index < 0)
                return std::nullopt;

            return Target { ParamTarget { node, index } };
        }();

        // The target this connection names is not in the tree it was saved with
        if (! target.has_value())
        {
            jassertfalse;
            continue;
        }

        entries.push_back ({ id, source, *target, (float) m[modulationIds::depth], (bool) m[modulationIds::bipolar] });
    }

    // The caller compiles once the whole graph is back
    mintModulations (entries);
}

void ModulationGraph::mintModulations (const std::vector<ModulationEntry>& entries)
{
    for (const auto& entry : entries)
    {
        // Ordered by ID, and a depth connection always outranks the one it targets, so
        // whatever an entry needs is already present by the time the entry is reached
        if (connections.find (entry.id) != connections.end() || ! modulatorRegistry.contains (entry.source) || ! canResolve (entry.target))
            continue;

        mintModulation (entry.source, entry.target, entry.depth, entry.bipolar, entry.id);
    }
}

void ModulationGraph::removeModulationsReferencing (ProcessorID node)
{
    std::vector<ConnectionID> stale;

    for (const auto& [id, connection] : connections)
    {
        const auto* param = std::get_if<ParamTarget> (&connection.target);

        if (param != nullptr && std::holds_alternative<ProcessorID> (param->node) && std::get<ProcessorID> (param->node) == node)
            stale.push_back (id);
    }

    for (const auto id : stale)
        removeConnection (id);
}

void ModulationGraph::removeModulationsReferencing (ModulatorID node)
{
    std::vector<ConnectionID> stale;

    for (const auto& [id, connection] : connections)
    {
        const auto* param = std::get_if<ParamTarget> (&connection.target);

        const bool isSource = connection.source == node;
        const bool isTarget = param != nullptr && std::holds_alternative<ModulatorID> (param->node) && std::get<ModulatorID> (param->node) == node;

        if (isSource || isTarget)
            stale.push_back (id);
    }

    for (const auto id : stale)
        removeConnection (id);
}

ConnectionID ModulationGraph::addConnection (ModulatorID source, Target target, float initialDepth, bool bipolar, std::optional<ConnectionID> explicitId)
{
    auto depth = std::make_shared<ModulatedValue> (juce::NormalisableRange<float> (-1.0f, 1.0f));
    depth->snapTo (initialDepth);

    if (prepared)
        depth->prepare (cachedSampleRate, cachedBlockSize);

    const auto id = explicitId.value_or (ConnectionID { nextId });

    // A connection already occupies this ID
    jassert (connections.find (id) == connections.end());

    // If a ConnectionID was specified, ensure the next non-specified ID will be above it.
    // This is required so that depth modulations always come after the connection they target.
    nextId = std::max (nextId, id.uid + 1);

    connections.emplace (id, Connection { source, std::move (target), bipolar, std::move (depth) });

    return id;
}

std::optional<ConnectionID>
    ModulationGraph::mintModulation (ModulatorID source, Target target, float initialDepth, bool bipolar, std::optional<ConnectionID> explicitId)
{
    if (const auto* param = std::get_if<ParamTarget> (&target))
    {
        // No modulated parameter at this index on the target node
        jassert (juce::isPositiveAndBelow (param->index, resolveNode (param->node).getModulatedParameters().size()));
    }

    if (findModulation (source, target).has_value())
        return std::nullopt;

    if (const auto owner = ownerNodeOf (target))
        if (const auto* modulator = std::get_if<ModulatorID> (&*owner))
            if (*modulator == source)
                return std::nullopt;

    return addConnection (source, std::move (target), initialDepth, bipolar, explicitId);
}

std::optional<ConnectionID>
    ModulationGraph::addModulation (ModulatorID source, Target target, float initialDepth, bool bipolar, std::optional<ConnectionID> explicitId)
{
    const auto id = mintModulation (source, std::move (target), initialDepth, bipolar, explicitId);

    if (id.has_value())
        compile();

    return id;
}

void ModulationGraph::removeConnection (ConnectionID id)
{
    if (connections.find (id) == connections.end())
        return;

    // Remove modulation connections targeting the depth of this one
    std::vector<ConnectionID> dependents;

    for (const auto& [otherId, connection] : connections)
    {
        const auto* depth = std::get_if<DepthTarget> (&connection.target);

        if (depth != nullptr && depth->connection == id)
            dependents.push_back (otherId);
    }

    for (const auto dependentId : dependents)
        removeConnection (dependentId);

    connections.erase (id);
}

bool ModulationGraph::contains (const NodeRef& node) const
{
    return std::visit (
        [this] (auto id)
        {
            if constexpr (std::is_same_v<std::decay_t<decltype (id)>, ProcessorID>)
                return processorGraph.contains (id);
            else
                return modulatorRegistry.contains (id);
        },
        node);
}

bool ModulationGraph::canResolve (const Target& target) const
{
    if (const auto* depth = std::get_if<DepthTarget> (&target))
        return connections.find (depth->connection) != connections.end();

    const auto& param = std::get<ParamTarget> (target);

    return contains (param.node) && juce::isPositiveAndBelow (param.index, resolveNode (param.node).getModulatedParameters().size());
}

std::optional<ConnectionID> ModulationGraph::findModulation (ModulatorID source, const Target& target) const
{
    for (const auto& [id, connection] : connections)
        if (connection.source == source && connection.target == target)
            return id;

    return std::nullopt;
}

std::vector<ModulationGraph::ModulationEntry> ModulationGraph::getModulationsFor (const Target& target) const
{
    std::vector<ModulationEntry> entries;

    for (const auto& [id, connection] : connections)
        if (connection.target == target)
            entries.push_back ({ id, connection.source, connection.target, connection.depth->getTargetValue(), connection.bipolar });

    return entries;
}

std::optional<NodeRef> ModulationGraph::ownerNodeOf (ConnectionID id) const
{
    auto current = id;

    while (true)
    {
        const auto it = connections.find (current);

        if (it == connections.end())
            return std::nullopt;

        if (const auto* param = std::get_if<ParamTarget> (&it->second.target))
            return param->node;

        const auto parent = std::get<DepthTarget> (it->second.target).connection;

        // Depth chains are strictly decreasing in ID, so this always terminates
        jassert (parent < current);
        current = parent;
    }
}

std::optional<NodeRef> ModulationGraph::ownerNodeOf (const Target& target) const
{
    if (const auto* param = std::get_if<ParamTarget> (&target))
        return param->node;

    return ownerNodeOf (std::get<DepthTarget> (target).connection);
}

Modulator& ModulationGraph::resolveSource (ModulatorID id) const { return modulatorRegistry.getModulator (id); }

const ParameterOwner& ModulationGraph::resolveNode (const NodeRef& node) const
{
    return std::visit (
        [this] (auto id) -> const ParameterOwner&
        {
            if constexpr (std::is_same_v<std::decay_t<decltype (id)>, ProcessorID>)
                return processorGraph.getProcessor (id);
            else
                return modulatorRegistry.getModulator (id);
        },
        node);
}

} // namespace cgo
