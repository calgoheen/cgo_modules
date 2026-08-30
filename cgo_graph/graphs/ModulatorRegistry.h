namespace cgo
{

class ModularGraph;

/** Owns the modulators of one ModularGraph and hands out ModulatorIDs. */
class ModulatorRegistry
{
public:
    struct Entry
    {
        ModulatorID id;
        Modulator* modulator;
    };

    ModulatorRegistry() = default;

    Modulator& getModulator (ModulatorID id) const;
    bool contains (ModulatorID id) const;
    std::vector<Entry> getAll() const;

private:
    friend class ModularGraph;

    struct Node
    {
        Node (ModulatorID idToUse, Modulator::Ptr modulatorToUse) : id (idToUse), modulator (std::move (modulatorToUse)) {}

        const ModulatorID id;
        const Modulator::Ptr modulator;
    };

    void prepare (double sampleRate, int maxBlockSize);
    void setPlayHead (juce::AudioPlayHead* newPlayHead);

    ModulatorID addModulator (std::unique_ptr<Modulator> modulator, std::optional<ModulatorID> id = std::nullopt);
    void removeModulator (ModulatorID id);

    juce::ValueTree toValueTree() const;
    void restoreFromValueTree (const juce::ValueTree& graphTree);

    Node* findNode (ModulatorID id) const;

    juce::OwnedArray<Node> nodes;

    juce::uint32 nextId = 0;

    juce::AudioPlayHead* playHead = nullptr;

    bool prepared = false;
    double cachedSampleRate = 0.0;
    int cachedBlockSize = 0;

    JUCE_DECLARE_NON_COPYABLE (ModulatorRegistry)
};

} // namespace cgo
