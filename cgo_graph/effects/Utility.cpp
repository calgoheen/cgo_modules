#include <cgo_graph/cgo_graph.h>

namespace cgo
{

CGO_ANON_NAMESPACE_BEGIN

juce::String stringFromPan (float val, int)
{
    const int amount = juce::roundToInt (std::abs (val) * 100.0f);

    if (amount == 0)
        return "C";

    return juce::String (val < 0.0f ? "L" : "R") + juce::String (amount);
}

float panFromString (const juce::String& str)
{
    const auto trimmed = str.trim();

    if (trimmed.startsWithIgnoreCase ("L"))
        return -trimmed.substring (1).getFloatValue() / 100.0f;

    if (trimmed.startsWithIgnoreCase ("R"))
        return trimmed.substring (1).getFloatValue() / 100.0f;

    return trimmed.getFloatValue() / 100.0f;
}

CGO_ANON_NAMESPACE_END

Utility::Utility()
  : Processor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true).withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
    params { .gain = addModulatedParameter (ParamUtils::createGainParameter ("gain", "Gain", -60.0f, 12.0f, 0.0f), {}, ModulatedValue::Rate::audio),
             .level = addModulatedParameter (ParamUtils::createPercentParameter ("level", "Level", 1.0f), {}, ModulatedValue::Rate::audio),
             .pan =
                 addModulatedParameter (ParamUtils::createRangedParameter ("pan", "Pan", "", { -1.0f, 1.0f }, 0.0f, ANON::stringFromPan, ANON::panFromString),
                                        {},
                                        ModulatedValue::Rate::audio),
             .width = addModulatedParameter (ParamUtils::createRangedParameter (
                                                 "width",
                                                 "Width",
                                                 "%",
                                                 { 0.0f, 2.0f },
                                                 1.0f,
                                                 [] (float val, int) { return juce::String (juce::roundToInt (val * 100.0f)); },
                                                 [] (const juce::String& str) { return str.getFloatValue() / 100.0f; }),
                                             {},
                                             ModulatedValue::Rate::audio),
             .mono = addModulatedParameter (ParamUtils::createBoolParameter ("mono", "Mono", false)),
             .invert = addModulatedParameter (ParamUtils::createBoolParameter ("invert", "Invert", false)) }
{
}

juce::String Utility::getTypeId() const { return "utility"; }

void Utility::prepareImpl() { utility.emplace (getNumChannels()); }

void Utility::processImpl (juce::AudioBuffer<float>& buffer)
{
    if (anyChanging (params.mono, params.invert))
        updateParameters();

    const auto channels = buffer.getArrayOfWritePointers();
    const int numSamples = buffer.getNumSamples();

    if (! anyChanging (params.gain, params.level, params.pan, params.width))
    {
        utility->process (channels, 0, numSamples);
        return;
    }

    for (int i = 0; i < numSamples; i++)
    {
        if (anyChanging (params.gain, params.level))
            utility->setGain (juce::Decibels::decibelsToGain (params.gain.getBuffer()[i]) * params.level.getBuffer()[i]);

        if (params.pan.isChanging())
            utility->setPan (params.pan.getBuffer()[i]);

        if (params.width.isChanging())
            utility->setWidth (params.width.getBuffer()[i]);

        utility->process (channels, i, 1);
    }
}

void Utility::updateParameters()
{
    utility->setMonoEnabled (ParamUtils::boolFromValue (params.mono.getCurrentValue()));
    utility->setPolarityInverted (ParamUtils::boolFromValue (params.invert.getCurrentValue()));
}

} // namespace cgo
