namespace cgo
{

class ParamHolder
{
public:
    std::unique_ptr<juce::AudioProcessorParameterGroup> createParameterGroup (const juce::String& groupId = "", const juce::String& groupName = "");
    void pushPrefixes (const juce::String& newIdPrefix, const juce::String& newNamePrefix);

protected:
    virtual ~ParamHolder() = default;

    template <typename... Args>
    void add (ParamUtils::ParamPtr& param, Args&&... args)
    {
        paramPtrs.push_back (&param);
        add (std::forward<Args> (args)...);
    }

    template <typename... Args>
    void add (ParamHolder& paramHolder, Args&&... args)
    {
        paramHolderPtrs.push_back (&paramHolder);
        add (std::forward<Args> (args)...);
    }

    void add() {}

private:
    std::atomic<bool> hasBeenCalled;

    juce::String idPrefix;
    juce::String namePrefix;

    std::vector<ParamUtils::ParamPtr*> paramPtrs;
    std::vector<ParamHolder*> paramHolderPtrs;
};

} // namespace cgo
