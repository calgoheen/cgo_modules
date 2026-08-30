#include <cgo_plugin/cgo_plugin.h>

namespace cgo::ParamUtils
{

std::unique_ptr<juce::AudioParameterFloat> createRangedParameter (const juce::String& id,
                                                                  const juce::String& name,
                                                                  const juce::String& label,
                                                                  const juce::NormalisableRange<float>& range,
                                                                  float defaultValue,
                                                                  std::function<juce::String (float, int)> stringFromValue,
                                                                  std::function<float (const juce::String&)> valueFromString)
{
    const auto attributes = juce::AudioParameterFloatAttributes()
                                .withStringFromValueFunction (std::move (stringFromValue))
                                .withValueFromStringFunction (std::move (valueFromString))
                                .withLabel (label);

    return std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { id, 1 }, name, range, defaultValue, attributes);
}

std::unique_ptr<juce::AudioParameterFloat> createPercentParameter (const juce::String& id, const juce::String& name, float defaultValue)
{
    auto stringFromValue = [] (float x, int) { return juce::String (juce::roundToInt (x * 100.0f)); };

    auto valueFromString = [] (const juce::String& str) { return str.getFloatValue() / 100.0f; };

    return createRangedParameter (id, name, "%", { 0.0f, 1.0f }, defaultValue, std::move (stringFromValue), std::move (valueFromString));
}

std::unique_ptr<juce::AudioParameterFloat>
    createGainParameter (const juce::String& id, const juce::String& name, float minGainDb, float maxGainDb, float defaultValue)
{
    auto stringFromValue = [] (float x, int) { return juce::String (x, 1); };

    auto valueFromString = [] (const juce::String& str) { return str.getFloatValue(); };

    return createRangedParameter (id, name, "dB", { minGainDb, maxGainDb }, defaultValue, std::move (stringFromValue), std::move (valueFromString));
}

std::unique_ptr<juce::AudioParameterFloat>
    createFreqParameter (const juce::String& id, const juce::String& name, float minFreq, float maxFreq, float centerFreq, float defaultValue)
{
    auto stringFromValue = [] (float x, int) { return juce::String (x, 2); };

    auto valueFromString = [] (const juce::String& str) { return str.getFloatValue(); };

    return createRangedParameter (id,
                                  name,
                                  "Hz",
                                  getRangeWithCenter (minFreq, maxFreq, centerFreq),
                                  defaultValue,
                                  std::move (stringFromValue),
                                  std::move (valueFromString));
}

std::unique_ptr<juce::AudioParameterFloat> createTimeParameter (const juce::String& id,
                                                                const juce::String& name,
                                                                float minTimeSeconds,
                                                                float maxTimeSeconds,
                                                                float centerTimeSeconds,
                                                                float defaultValue)
{
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
                                  "",
                                  getRangeWithCenter (minTimeSeconds, maxTimeSeconds, centerTimeSeconds),
                                  defaultValue,
                                  std::move (stringFromValue),
                                  std::move (valueFromString));
}

std::unique_ptr<juce::AudioParameterChoice>
    createChoiceParameter (const juce::String& id, const juce::String& name, const juce::StringArray& choices, int defaultChoice)
{
    auto valueFromString = [=] (const juce::String& str) { return choices.indexOf (str, true); };

    const auto attributes = juce::AudioParameterChoiceAttributes().withValueFromStringFunction (std::move (valueFromString));

    return std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { id, 1 }, name, choices, defaultChoice, attributes);
}

std::unique_ptr<juce::AudioParameterChoice> createBoolParameter (const juce::String& id, const juce::String& name, bool defaultValue)
{
    return createChoiceParameter (id, name, { "Off", "On" }, (int) defaultValue);
}

std::unique_ptr<juce::AudioParameterChoice> createSyncedRateParameter (const juce::String& id, const juce::String& name, const juce::String& defaultRate)
{
    static const juce::StringArray choices { "1/32", "1/24", "1/16", "1/12", "1/8", "1/6", "3/16", "1/4", "5/16", "1/3",
                                             "3/8",  "1/2",  "3/4",  "1",    "2",   "3",   "4",    "6",   "8" };

    const int defaultChoice = [&]
    {
        if (choices.contains (defaultRate))
            return choices.indexOf (defaultRate);

        return choices.indexOf ("2");
    }();

    return createChoiceParameter (id, name, choices, defaultChoice);
}

bool boolFromValue (float value) { return choiceFromValue (value) == 1; }

double syncedRateIndexToBeats (int index)
{
    static const auto periodInBeats = std::array { 1.0 / 8.0, 1.0 / 6.0, 1.0 / 4.0, 1.0 / 3.0, 1.0 / 2.0, 2.0 / 3.0, 3.0 / 4.0, 1.0,  5.0 / 4.0, 4.0 / 3.0,
                                                   3.0 / 2.0, 2.0,       3.0,       4.0,       8.0,       12.0,      16.0,      24.0, 32.0 };

    jassert (juce::isPositiveAndBelow (index, (int) periodInBeats.size()));
    return periodInBeats[(size_t) index];
}

juce::NormalisableRange<float> getRangeWithCenter (float min, float max, float center)
{
    juce::NormalisableRange<float> range { min, max };
    range.setSkewForCentre (center);
    return range;
}

} // namespace cgo::ParamUtils
