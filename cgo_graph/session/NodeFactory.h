namespace cgo
{

/** Turns a saved type ID back into a node.

    @see Processor::getTypeId
*/
class NodeFactory
{
public:
    struct NodeType
    {
        juce::String typeId;
        juce::String displayName;
    };

    static std::vector<NodeType> getProcessorTypes();
    static std::vector<NodeType> getModulatorTypes();
    static juce::String getDisplayName (const juce::String& typeId);

    static std::unique_ptr<Processor> createProcessor (const juce::String& typeId);
    static std::unique_ptr<Modulator> createModulator (const juce::String& typeId);
};

} // namespace cgo
