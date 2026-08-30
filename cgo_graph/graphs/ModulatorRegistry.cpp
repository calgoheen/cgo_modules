#include <cgo_graph/cgo_graph.h>

namespace cgo
{

namespace modulatorIds
{

const juce::Identifier MODULATORS { "MODULATORS" };
const juce::Identifier NODE { "NODE" };
const juce::Identifier id { "id" };
const juce::Identifier type { "type" };

} // namespace modulatorIds

Modulator& ModulatorRegistry::getModulator (ModulatorID id) const
{
    auto* node = findNode (id);
    jassert (node != nullptr);
    return *node->modulator;
}

bool ModulatorRegistry::contains (ModulatorID id) const { return findNode (id) != nullptr; }

std::vector<ModulatorRegistry::Entry> ModulatorRegistry::getAll() const
{
    std::vector<Entry> entries;
    entries.reserve ((size_t) nodes.size());

    for (auto* node : nodes)
        entries.push_back ({ node->id, node->modulator.get() });

    return entries;
}

void ModulatorRegistry::prepare (double sampleRate, int blockSize)
{
    cachedSampleRate = sampleRate;
    cachedBlockSize = blockSize;
    prepared = true;

    for (auto* node : nodes)
        node->modulator->prepare (sampleRate, blockSize);
}

void ModulatorRegistry::setPlayHead (juce::AudioPlayHead* newPlayHead)
{
    playHead = newPlayHead;

    for (auto* node : nodes)
        node->modulator->setPlayHead (newPlayHead);
}

ModulatorID ModulatorRegistry::addModulator (std::unique_ptr<Modulator> modulator, std::optional<ModulatorID> explicitId)
{
    if (! explicitId.has_value())
    {
        while (findNode (ModulatorID { nextId }) != nullptr)
            ++nextId;

        explicitId = ModulatorID { nextId };
    }

    const auto id = *explicitId;

    // A modulator already occupies this ID
    jassert (findNode (id) == nullptr);
    auto* node = nodes.add (new Node (id, modulator.release()));

    node->modulator->setPlayHead (playHead);

    if (prepared)
        node->modulator->prepare (cachedSampleRate, cachedBlockSize);

    return id;
}

void ModulatorRegistry::removeModulator (ModulatorID id)
{
    auto* node = findNode (id);
    jassert (node != nullptr);

    nodes.removeObject (node);
}

juce::ValueTree ModulatorRegistry::toValueTree() const
{
    juce::ValueTree tree { modulatorIds::MODULATORS };

    for (auto* node : nodes)
    {
        juce::ValueTree n { modulatorIds::NODE };

        n.setProperty (modulatorIds::id, (int) node->id.uid, nullptr);
        n.setProperty (modulatorIds::type, node->modulator->getTypeId(), nullptr);
        n.appendChild (node->modulator->parametersToValueTree(), nullptr);

        tree.appendChild (n, nullptr);
    }

    return tree;
}

void ModulatorRegistry::restoreFromValueTree (const juce::ValueTree& graphTree)
{
    jassert (nodes.isEmpty());

    for (const auto n : graphTree.getChildWithName (modulatorIds::MODULATORS))
    {
        if (! n.hasType (modulatorIds::NODE))
            continue;

        auto modulator = NodeFactory::createModulator (n[modulatorIds::type].toString());

        // Unknown type ID
        if (modulator == nullptr)
        {
            jassertfalse;
            continue;
        }

        const ModulatorID id { (juce::uint32) (int) n[modulatorIds::id] };

        modulator->restoreParameters (n);
        addModulator (std::move (modulator), id);
    }
}

ModulatorRegistry::Node* ModulatorRegistry::findNode (ModulatorID id) const
{
    for (auto* node : nodes)
        if (node->id == id)
            return node;

    return nullptr;
}

} // namespace cgo
