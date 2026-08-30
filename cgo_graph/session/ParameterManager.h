namespace cgo
{

class ModularGraph;

class ParameterManager
{
public:
    ParameterManager (juce::AudioProcessor& processor, int numGenericParameters = 1024);
    ~ParameterManager();

    /** Links the parameters of owner to the host. */
    void attach (ParameterOwner& owner);

    /** Unlinks all parameters of owner from the host. */
    void detach (const ParameterOwner& owner);

    /** Returns the number of parameters can still be exposed to the host. */
    int getNumFreeSlots() const noexcept;

    /** Unlinks every slot and returns the whole pool to free. */
    void reset();

    /** The host slot behind each of owner's parameters, by parameter index, with -1 where a
        parameter has none. Empty if the owner is not attached. Pairs with restoreLink: what
        this returns is what puts a detached owner back on the same slots.
    */
    std::vector<int> getSlots (const ParameterOwner& owner) const;

    /** Links one named slot to one named parameter, rather than taking whatever the pool
        hands out.
    */
    void restoreLink (int slotIndex, ParameterOwner& owner, const juce::String& paramID);

    /** Serializes the current state. */
    juce::ValueTree toValueTree (const ModularGraph& graph) const;

    /** Restores from saved state. */
    void restoreFromValueTree (const juce::ValueTree& parent, ModularGraph& graph);

    juce::AudioProcessorValueTreeState& getValueTreeState() noexcept { return apvts; }

private:
    class GenericParameter;

    static constexpr int unassignedSlot = -1;

    static juce::AudioProcessorValueTreeState::ParameterLayout buildLayout (int numGenericParameters);

    juce::AudioProcessorValueTreeState apvts;
    std::vector<GenericParameter*> genericParameters;
    std::vector<int> freeSlots;
    std::unordered_map<const ParameterOwner*, std::vector<int>> ownerSlots;

    JUCE_DECLARE_NON_COPYABLE (ParameterManager)
};

} // namespace cgo
