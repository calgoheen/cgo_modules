namespace cgo
{

/** One complete graph: a ProcessorGraph holding the processors and their audio wiring, a
    ModulatorRegistry holding the modulators, and a ModulationGraph holding the modulation
    between them.

    The host block is rendered in sub-blocks of at most subBlockSize samples, with the
    modulators advanced once per sub-block.
*/
class ModularGraph final : public BasicAudioProcessor
{
public:
    static constexpr int defaultSubBlockSize = 64;

    ModularGraph (bool isSynth, int numSidechainChannels = 0, int subBlockSize = defaultSubBlockSize);

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void setPlayHead (juce::AudioPlayHead* newPlayHead) override;

    ProcessorGraph& processors() { return processorGraph; }
    const ProcessorGraph& processors() const { return processorGraph; }

    ModulatorRegistry& modulators() { return modulatorRegistry; }
    const ModulatorRegistry& modulators() const { return modulatorRegistry; }

    ModulationGraph& modulation() { return modulationGraph; }
    const ModulationGraph& modulation() const { return modulationGraph; }

    /** Returns true if the given node exists in this graph. */
    bool contains (NodeRef node) const;

    /** Access the parameters of a node in this graph. */
    ParameterOwner& getNode (NodeRef node) const;

    /** The node's parameter at this index, or nullptr if either the node or the index is gone. */
    ModulatedParameter* findModulatedParameter (NodeRef node, int index) const;

    /** Adds a modulator and recompiles. An explicit ID may be given but the caller should
        ensure it does not already exist in the graph.
    */
    ModulatorID addModulator (std::unique_ptr<Modulator> modulator, std::optional<ModulatorID> explicitId = std::nullopt);

    /** Removes a modulator along with every connection referencing it. */
    void removeModulator (ModulatorID id);

    /** Adds a processor and recompiles. An explicit ID may be given but the caller should
        ensure it does not already exist in the graph.
    */
    ProcessorID addProcessor (std::unique_ptr<Processor> processor, std::optional<ProcessorID> explicitId = std::nullopt);

    /** Removes a processor along with every connection referencing it. */
    void removeProcessor (ProcessorID id);

    /** Serializes the entire graph. */
    juce::ValueTree toValueTree() const;

    /** Restores the entire graph. Must be called on an empty graph. */
    void restoreFromValueTree (const juce::ValueTree& parent);

private:
    class SubBlockPlayHead final : public juce::AudioPlayHead
    {
    public:
        juce::Optional<PositionInfo> getPosition() const override;
        bool canControlTransport() override;
        void transportPlay (bool shouldStartPlaying) override;
        void transportRecord (bool shouldStartRecording) override;
        void transportRewind() override;

        void setSource (juce::AudioPlayHead* newSource);
        void prepare (double newSampleRate);
        void setOffset (int startSample);

    private:
        juce::AudioPlayHead* source = nullptr;
        double sampleRate = 0.0;
        int offset = 0;
    };

    static BusesProperties getBusesProperties (bool isSynth, int numSidechainChannels);
    BusesLayout getMainBusesLayout (int numSidechainChannels) const;

    bool isBusesLayoutSupported (const BusesLayout&) const override;

    const int subBlockSize;

    ProcessorGraph processorGraph;
    ModulatorRegistry modulatorRegistry;
    ModulationGraph modulationGraph;

    SubBlockPlayHead subBlockPlayHead;
    juce::MidiBuffer subBlockMidi;

    JUCE_DECLARE_NON_COPYABLE (ModularGraph)
};

} // namespace cgo
