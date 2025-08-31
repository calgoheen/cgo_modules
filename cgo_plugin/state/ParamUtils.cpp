#include <cgo_plugin/cgo_plugin.h>

namespace cgo
{

ParamUtils::ParamPtr ParamUtils::createRangedParameter (const juce::String& id,
                                                        const juce::String& name,
                                                        const juce::String& label,
                                                        const juce::NormalisableRange<float>& range,
                                                        float defaultValue,
                                                        std::function<juce::String (float, int)> stringFromValue,
                                                        std::function<float (const juce::String&)> valueFromString)
{
    const auto attributes = juce::AudioParameterFloatAttributes().withStringFromValueFunction (std::move (stringFromValue)).withValueFromStringFunction (std::move (valueFromString)).withLabel (label);

    return std::unique_ptr<juce::RangedAudioParameter> (new juce::AudioParameterFloat (juce::ParameterID { id, 1 },
                                                                                       name,
                                                                                       range,
                                                                                       defaultValue,
                                                                                       attributes));
}

ParamUtils::ParamPtr ParamUtils::createPercentParameter (const juce::String& id,
                                                         const juce::String& name,
                                                         float defaultValue)
{
    auto stringFromValue = [] (float x, int)
    {
        return juce::String (juce::roundToInt (x * 100.0f));
    };

    auto valueFromString = [] (const juce::String& str)
    {
        return str.getFloatValue() / 100.0f;
    };

    return createRangedParameter (id,
                                  name, 
                                  "%", 
                                  { 0.0f, 1.0f }, 
                                  defaultValue, 
                                  std::move (stringFromValue), 
                                  std::move (valueFromString));
}

ParamUtils::ParamPtr ParamUtils::createGainParameter (const juce::String& id,
                                                      const juce::String& name,
                                                      float minGainDb,
                                                      float maxGainDb,
                                                      float defaultValue)
{
    auto stringFromValue = [] (float x, int)
    {
        return juce::String (x, 1);
    };

    auto valueFromString = [] (const juce::String& str)
    {
        return str.getFloatValue();
    };

    return createRangedParameter (id,
                                  name, 
                                  "dB", 
                                  { minGainDb, maxGainDb }, 
                                  defaultValue,
                                  std::move (stringFromValue), 
                                  std::move (valueFromString));
}

ParamUtils::ParamPtr ParamUtils::createFreqParameter (const juce::String& id,
                                                      const juce::String& name,
                                                      float minFreq,
                                                      float maxFreq,
                                                      float centerFreq,
                                                      float defaultValue)
{
    auto stringFromValue = [] (float x, int)
    {
        return juce::String (x, 2);
    };

    auto valueFromString = [] (const juce::String& str)
    {
        return str.getFloatValue();
    };

    return createRangedParameter (id,
                                  name, 
                                  "Hz", 
                                  getRangeWithCenter (minFreq, maxFreq, centerFreq), 
                                  defaultValue, 
                                  std::move (stringFromValue), 
                                  std::move (valueFromString));
}

ParamUtils::ParamPtr ParamUtils::createTimeParameter (const juce::String& id,
                                                      const juce::String& name,
                                                      float minTimeSeconds,
                                                      float maxTimeSeconds,
                                                      float centerTimeSeconds,
                                                      float defaultValue)
{
    juce::NormalisableRange<float> range { minTimeSeconds, maxTimeSeconds };
    range.setSkewForCentre (centerTimeSeconds);

    auto stringFromValue = [] (float x, int)
    {
        if (x < 1.0f)
            return juce::String (x * 1e3f, 1) + " ms";
        else
            return juce::String (x, 2) + " s";
    };

    auto valueFromString = [] (const juce::String& str)
    {
        if (str.endsWith (" ms"))
            return str.dropLastCharacters (3).getFloatValue() / 1e3f;
        else if (str.endsWith (" s"))
            return str.dropLastCharacters (2).getFloatValue();
        else
            return str.getFloatValue() / 1e3f;
    };

    return createRangedParameter (id,
                                  name, 
                                  "Hz", 
                                  getRangeWithCenter (minTimeSeconds, maxTimeSeconds, centerTimeSeconds), 
                                  defaultValue, 
                                  std::move (stringFromValue), 
                                  std::move (valueFromString));
}

ParamUtils::ParamPtr ParamUtils::createChoiceParameter (const juce::String& id,
                                                        const juce::String& name,
                                                        const juce::StringArray& choices,
                                                        int defaultChoice)
{
    auto stringFromValue = [=] (float x, int)
    {
        return choices[int (x)];
    };

    auto valueFromString = [=] (const juce::String& str)
    {
        const int index = choices.indexOf (str, true);
        return index >= 0 ? static_cast<float> (index) : 0.0f;
    };

    return createRangedParameter (id,
                                  name, 
                                  "", 
                                  { 0.0f, float (choices.size() - 1), 1.0f }, 
                                  static_cast<float> (defaultChoice), 
                                  std::move (stringFromValue), 
                                  std::move (valueFromString));
}

ParamUtils::ParamPtr ParamUtils::createBoolParameter (const juce::String& id,
                                                      const juce::String& name,
                                                      bool defaultValue)
{
    return createChoiceParameter (id, name, { "Off", "On" }, static_cast<int> (defaultValue));
}

ParamUtils::ParamPtr ParamUtils::createSyncedRateParameter (const juce::String& id,
                                                            const juce::String& name)
{
    static const juce::StringArray choices { "1/32", "1/24", "1/16", "1/12", "1/8", "1/6", "3/16", "1/4", "5/16", "1/3", "3/8", "1/2", "3/4", "1", "2", "3", "4", "6", "8" };
    return createChoiceParameter (id, name, choices, 13);
}

double ParamUtils::syncedRateIndexToBeats (int index)
{
    static const auto periodInBeats = std::array { 1.0 / 8.0, 1.0 / 6.0, 1.0 / 4.0, 1.0 / 3.0, 1.0 / 2.0, 2.0 / 3.0, 3.0 / 4.0, 1.0, 5.0 / 4.0, 4.0 / 3.0, 3.0 / 2.0, 2.0, 3.0, 4.0, 8.0, 12.0, 16.0, 24.0, 32.0 };
    return periodInBeats[index];
}

juce::NormalisableRange<float> ParamUtils::getRangeWithCenter (float min, float max, float center)
{
    juce::NormalisableRange<float> range { min, max };
    range.setSkewForCentre (center);
    return range;
}

} // namespace cgo
