namespace cgo
{

struct ParamUtils
{
    typedef OptionalPointer<juce::RangedAudioParameter> ParamPtr;

    static ParamPtr createRangedParameter (const juce::String& id,
                                           const juce::String& name,
                                           const juce::String& label,
                                           const juce::NormalisableRange<float>& range,
                                           float defaultValue,
                                           std::function<juce::String (float, int)> stringFromValue = nullptr,
                                           std::function<float (const juce::String&)> valueFromString = nullptr);

    static ParamPtr createPercentParameter (const juce::String& id,
                                            const juce::String& name,
                                            float defaultValue);

    static ParamPtr createGainParameter (const juce::String& id,
                                         const juce::String& name,
                                         float minGainDb,
                                         float maxGainDb,
                                         float defaultValue);

    static ParamPtr createFreqParameter (const juce::String& id,
                                         const juce::String& name,
                                         float minFreq,
                                         float maxFreq,
                                         float centerFreq,
                                         float defaultValue);

    static ParamPtr createTimeParameter (const juce::String& id,
                                         const juce::String& name,
                                         float minTimeSeconds,
                                         float maxTimeSeconds,
                                         float centerTimeSeconds,
                                         float defaultValue);

    static ParamPtr createChoiceParameter (const juce::String& id,
                                           const juce::String& name,
                                           const juce::StringArray& choices,
                                           int defaultChoice);

    static ParamPtr createBoolParameter (const juce::String& id,
                                         const juce::String& name,
                                         bool defaultValue);

    static ParamPtr createSyncedRateParameter (const juce::String& id,
                                               const juce::String& name);

    static double syncedRateIndexToBeats (int index);

    static juce::NormalisableRange<float> getRangeWithCenter (float min, float max, float center);
};

} // namespace cgo
