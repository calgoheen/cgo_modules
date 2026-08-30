namespace cgo
{

class ModularGraph;
class ModulationGraph;

/** Owns the processors of one ModularGraph and the audio wiring between them, and hands
    out ProcessorIDs. Wraps a juce::AudioProcessorGraph, which manages the connections and
    graph building.
*/
class ProcessorGraph
{
public:
    struct Entry
    {
        ProcessorID id;
        Processor* processor;
    };

    struct Connection
    {
        std::optional<ProcessorID> source; // nullopt = graph audio input
        std::optional<ProcessorID> destination; // nullopt = graph audio output
    };

    ProcessorGraph (const juce::AudioProcessor::BusesLayout& layout);

    Processor& getProcessor (ProcessorID id) const;
    bool contains (ProcessorID id) const;
    std::vector<Entry> getAll() const;

    std::vector<Connection> getConnections() const;

    void connect (ProcessorID source, ProcessorID destination);
    void connectToInput (ProcessorID node);
    void connectToOutput (ProcessorID node);
    void disconnect (ProcessorID source, ProcessorID destination);
    void disconnectFromInput (ProcessorID node);
    void disconnectFromOutput (ProcessorID node);
    void clearConnections();

    void connectInputToOutput();
    void disconnectInputFromOutput();

private:
    friend class ModularGraph;
    friend class ModulationGraph;

    void setPlayHead (juce::AudioPlayHead* newPlayHead);
    void prepare (double sampleRate, int blockSize);
    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi);

    ProcessorID addProcessor (std::unique_ptr<Processor> processor, std::optional<ProcessorID> explicitId);
    void removeProcessor (ProcessorID id);

    std::vector<juce::AudioProcessorGraph::Node::Ptr> getProcessorNodes() const;

    juce::ValueTree toValueTree() const;
    void restoreFromValueTree (const juce::ValueTree& graphTree);

    bool isGraphIO (ProcessorID id) const;

    const int numChannels;
    const int numPassThroughChannels;

    juce::AudioProcessorGraph audioGraph;

    ProcessorID audioInputNode;
    ProcessorID audioOutputNode;
    ProcessorID midiInputNode;

    JUCE_DECLARE_NON_COPYABLE (ProcessorGraph)
};

} // namespace cgo
