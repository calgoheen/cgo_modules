#include <cgo_graph/cgo_graph.h>

namespace cgo
{

namespace processorIds
{

const juce::Identifier PROCESSORS { "PROCESSORS" };
const juce::Identifier NODE { "NODE" };
const juce::Identifier AUDIO { "AUDIO" };
const juce::Identifier C { "C" };
const juce::Identifier id { "id" };
const juce::Identifier type { "type" };
const juce::Identifier src { "src" };
const juce::Identifier dst { "dst" };

constexpr int graphIO = -1;

} // namespace processorIds

constexpr auto graphUpdateKind = juce::AudioProcessorGraph::UpdateKind::async;

ProcessorGraph::ProcessorGraph (const juce::AudioProcessor::BusesLayout& layout)
  : numChannels (layout.getMainOutputChannels()), numPassThroughChannels (juce::jmin (layout.getMainInputChannels(), layout.getMainOutputChannels()))
{
    using IOProcessor = juce::AudioProcessorGraph::AudioGraphIOProcessor;

    audioGraph.setBusesLayout (layout);

    audioInputNode = audioGraph.addNode (std::make_unique<IOProcessor> (IOProcessor::audioInputNode))->nodeID;
    audioOutputNode = audioGraph.addNode (std::make_unique<IOProcessor> (IOProcessor::audioOutputNode))->nodeID;
    midiInputNode = audioGraph.addNode (std::make_unique<IOProcessor> (IOProcessor::midiInputNode))->nodeID;
}

Processor& ProcessorGraph::getProcessor (ProcessorID id) const
{
    auto* node = audioGraph.getNodeForId (id);

    jassert (node != nullptr);
    return *static_cast<Processor*> (node->getProcessor());
}

bool ProcessorGraph::contains (ProcessorID id) const
{
    if (isGraphIO (id))
        return false;

    return audioGraph.getNodeForId (id) != nullptr;
}

std::vector<ProcessorGraph::Entry> ProcessorGraph::getAll() const
{
    std::vector<Entry> entries;
    entries.reserve ((size_t) audioGraph.getNumNodes());

    for (auto* node : audioGraph.getNodes())
    {
        const auto id = node->nodeID;

        if (isGraphIO (id))
            continue;

        entries.push_back ({ id, static_cast<Processor*> (node->getProcessor()) });
    }

    return entries;
}

std::vector<ProcessorGraph::Connection> ProcessorGraph::getConnections() const
{
    auto endpoint = [this] (ProcessorID id) -> std::optional<ProcessorID>
    {
        if (id == audioInputNode || id == audioOutputNode)
            return std::nullopt;

        return id;
    };

    std::vector<Connection> connections;

    for (const auto& c : audioGraph.getConnections())
    {
        if (c.source.channelIndex == juce::AudioProcessorGraph::midiChannelIndex)
            continue;

        const Connection edge { endpoint (c.source.nodeID), endpoint (c.destination.nodeID) };

        const bool seen = std::any_of (connections.begin(),
                                       connections.end(),
                                       [&] (const Connection& e) { return e.source == edge.source && e.destination == edge.destination; });

        if (! seen)
            connections.push_back (edge);
    }

    return connections;
}

void ProcessorGraph::connect (ProcessorID source, ProcessorID destination)
{
    for (int i = 0; i < numChannels; i++)
        if (! audioGraph.addConnection ({ { source, i }, { destination, i } }, graphUpdateKind))
            jassertfalse;
}

void ProcessorGraph::connectToInput (ProcessorID node)
{
    for (int i = 0; i < numChannels; i++)
        if (! audioGraph.addConnection ({ { audioInputNode, i }, { node, i } }, graphUpdateKind))
            jassertfalse;
}

void ProcessorGraph::connectToOutput (ProcessorID node)
{
    for (int i = 0; i < numChannels; i++)
        if (! audioGraph.addConnection ({ { node, i }, { audioOutputNode, i } }, graphUpdateKind))
            jassertfalse;
}

void ProcessorGraph::disconnect (ProcessorID source, ProcessorID destination)
{
    for (int i = 0; i < numChannels; i++)
        if (! audioGraph.removeConnection ({ { source, i }, { destination, i } }, graphUpdateKind))
            jassertfalse;
}

void ProcessorGraph::disconnectFromInput (ProcessorID node)
{
    for (int i = 0; i < numChannels; i++)
        if (! audioGraph.removeConnection ({ { audioInputNode, i }, { node, i } }, graphUpdateKind))
            jassertfalse;
}

void ProcessorGraph::disconnectFromOutput (ProcessorID node)
{
    for (int i = 0; i < numChannels; i++)
        if (! audioGraph.removeConnection ({ { node, i }, { audioOutputNode, i } }, graphUpdateKind))
            jassertfalse;
}

void ProcessorGraph::clearConnections()
{
    for (const auto& c : audioGraph.getConnections())
        if (c.source.channelIndex != juce::AudioProcessorGraph::midiChannelIndex)
            audioGraph.removeConnection (c, graphUpdateKind);
}

void ProcessorGraph::connectInputToOutput()
{
    for (int i = 0; i < numPassThroughChannels; i++)
        audioGraph.addConnection ({ { audioInputNode, i }, { audioOutputNode, i } }, graphUpdateKind);
}

void ProcessorGraph::disconnectInputFromOutput()
{
    for (int i = 0; i < numPassThroughChannels; i++)
        audioGraph.removeConnection ({ { audioInputNode, i }, { audioOutputNode, i } }, graphUpdateKind);
}

void ProcessorGraph::setPlayHead (juce::AudioPlayHead* newPlayHead) { audioGraph.setPlayHead (newPlayHead); }

void ProcessorGraph::prepare (double sampleRate, int blockSize)
{
    audioGraph.setRateAndBufferSizeDetails (sampleRate, blockSize);
    audioGraph.prepareToPlay (sampleRate, blockSize);
}

void ProcessorGraph::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) { audioGraph.processBlock (buffer, midi); }

ProcessorID ProcessorGraph::addProcessor (std::unique_ptr<Processor> processor, std::optional<ProcessorID> explicitId)
{
    return audioGraph.addNode (std::move (processor), explicitId, graphUpdateKind)->nodeID;
}

void ProcessorGraph::removeProcessor (ProcessorID id) { audioGraph.removeNode (id, graphUpdateKind); }

std::vector<juce::AudioProcessorGraph::Node::Ptr> ProcessorGraph::getProcessorNodes() const
{
    std::vector<juce::AudioProcessorGraph::Node::Ptr> result;
    result.reserve ((size_t) audioGraph.getNumNodes());

    for (auto* node : audioGraph.getNodes())
    {
        if (isGraphIO (node->nodeID))
            continue;

        result.push_back (node);
    }

    return result;
}

juce::ValueTree ProcessorGraph::toValueTree() const
{
    juce::ValueTree tree { processorIds::PROCESSORS };

    for (const auto& entry : getAll())
    {
        juce::ValueTree node { processorIds::NODE };

        node.setProperty (processorIds::id, (int) entry.id.uid, nullptr);
        node.setProperty (processorIds::type, entry.processor->getTypeId(), nullptr);
        node.appendChild (entry.processor->parametersToValueTree(), nullptr);

        tree.appendChild (node, nullptr);
    }

    juce::ValueTree audio { processorIds::AUDIO };

    for (const auto& connection : getConnections())
    {
        if (! connection.source.has_value() && ! connection.destination.has_value())
            continue;

        juce::ValueTree c { processorIds::C };

        c.setProperty (processorIds::src, connection.source.has_value() ? (int) connection.source->uid : processorIds::graphIO, nullptr);
        c.setProperty (processorIds::dst, connection.destination.has_value() ? (int) connection.destination->uid : processorIds::graphIO, nullptr);

        audio.appendChild (c, nullptr);
    }

    tree.appendChild (audio, nullptr);

    return tree;
}

void ProcessorGraph::restoreFromValueTree (const juce::ValueTree& graphTree)
{
    jassert (getAll().empty());

    const auto tree = graphTree.getChildWithName (processorIds::PROCESSORS);

    for (const auto node : tree)
    {
        if (! node.hasType (processorIds::NODE))
            continue;

        auto processor = NodeFactory::createProcessor (node[processorIds::type].toString());

        // Unknown type ID
        if (processor == nullptr)
        {
            jassertfalse;
            continue;
        }

        const ProcessorID id { (juce::uint32) (int) node[processorIds::id] };

        // Restored before the node joins the graph, since adding it prepares it and prepareImpl reads getCurrentValue()
        processor->restoreParameters (node);
        addProcessor (std::move (processor), id);
    }

    auto isRestorable = [this] (int uid) { return uid == processorIds::graphIO || contains (ProcessorID { (juce::uint32) uid }); };

    for (const auto c : tree.getChildWithName (processorIds::AUDIO))
    {
        const int source = c[processorIds::src];
        const int destination = c[processorIds::dst];

        if (! isRestorable (source) || ! isRestorable (destination))
            continue;

        if (source == processorIds::graphIO)
            connectToInput (ProcessorID { (juce::uint32) destination });
        else if (destination == processorIds::graphIO)
            connectToOutput (ProcessorID { (juce::uint32) source });
        else
            connect (ProcessorID { (juce::uint32) source }, ProcessorID { (juce::uint32) destination });
    }
}

bool ProcessorGraph::isGraphIO (ProcessorID id) const { return id == audioInputNode || id == audioOutputNode || id == midiInputNode; }

} // namespace cgo
