namespace cgo::ParamUtils
{

inline constexpr int defaultSyncedRateIndex = 14;

std::unique_ptr<juce::AudioParameterFloat> createRangedParameter (const juce::String& id,
                                                                  const juce::String& name,
                                                                  const juce::String& label,
                                                                  const juce::NormalisableRange<float>& range,
                                                                  float defaultValue,
                                                                  std::function<juce::String (float, int)> stringFromValue = nullptr,
                                                                  std::function<float (const juce::String&)> valueFromString = nullptr);

std::unique_ptr<juce::AudioParameterFloat> createPercentParameter (const juce::String& id, const juce::String& name, float defaultValue);

std::unique_ptr<juce::AudioParameterFloat>
    createGainParameter (const juce::String& id, const juce::String& name, float minGainDb, float maxGainDb, float defaultValue);

std::unique_ptr<juce::AudioParameterFloat>
    createFreqParameter (const juce::String& id, const juce::String& name, float minFreq, float maxFreq, float centerFreq, float defaultValue);

std::unique_ptr<juce::AudioParameterFloat> createTimeParameter (const juce::String& id,
                                                                const juce::String& name,
                                                                float minTimeSeconds,
                                                                float maxTimeSeconds,
                                                                float centerTimeSeconds,
                                                                float defaultValue);

std::unique_ptr<juce::AudioParameterChoice>
    createChoiceParameter (const juce::String& id, const juce::String& name, const juce::StringArray& choices, int defaultChoice);

std::unique_ptr<juce::AudioParameterChoice> createBoolParameter (const juce::String& id, const juce::String& name, bool defaultValue);

std::unique_ptr<juce::AudioParameterChoice> createSyncedRateParameter (const juce::String& id, const juce::String& name, const juce::String& defaultRate = "");

bool boolFromValue (float value);

template <typename T = int>
T choiceFromValue (float value)
{
    return static_cast<T> (juce::roundToInt (value));
}

double syncedRateIndexToBeats (int index);

juce::NormalisableRange<float> getRangeWithCenter (float min, float max, float center);

} // namespace cgo::ParamUtils
