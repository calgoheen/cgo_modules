#include <cgo_graph/cgo_graph.h>

namespace cgo
{

Gate::Gate()
  : Processor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true).withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
    params { .threshold = addModulatedParameter (ParamUtils::createGainParameter ("threshold", "Threshold", -80.0f, 0.0f, -40.0f)),
             .hysteresis = addModulatedParameter (ParamUtils::createRangedParameter (
                 "hyst",
                 "Hysteresis",
                 "dB",
                 { 0.0f, 24.0f },
                 3.0f,
                 [] (float val, int) { return juce::String (val, 1); },
                 [] (const juce::String& str) { return str.getFloatValue(); })),
             .range = addModulatedParameter (ParamUtils::createGainParameter ("range", "Range", -100.0f, 0.0f, -100.0f)),
             .attack = addModulatedParameter (ParamUtils::createTimeParameter ("attack", "Attack", 1e-4f, 0.5f, 5e-3f, 1e-3f)),
             .hold = addModulatedParameter (ParamUtils::createTimeParameter ("hold", "Hold", 0.0f, 1.0f, 0.05f, 0.01f)),
             .release = addModulatedParameter (ParamUtils::createTimeParameter ("release", "Release", 1e-3f, 2.0f, 0.1f, 0.1f)),
             .mode = addModulatedParameter (ParamUtils::createChoiceParameter ("mode", "Mode", { "Peak", "RMS" }, 0)) }
{
}

juce::String Gate::getTypeId() const { return "gate"; }

void Gate::prepareImpl() { gate.emplace ((float) getSampleRate(), getNumChannels()); }

void Gate::processImpl (juce::AudioBuffer<float>& buffer)
{
    if (anyChanging (params.attack, params.hold, params.release, params.mode))
        updateBallistics();

    const auto channels = buffer.getArrayOfWritePointers();
    const int numSamples = buffer.getNumSamples();

    if (! anyChanging (params.threshold, params.hysteresis, params.range))
    {
        gate->process (channels, 0, numSamples);
        return;
    }

    for (int i = 0; i < numSamples; i++)
    {
        if (params.threshold.isChanging())
            gate->setThreshold (params.threshold.getBuffer()[i]);

        if (params.hysteresis.isChanging())
            gate->setHysteresis (params.hysteresis.getBuffer()[i]);

        if (params.range.isChanging())
            gate->setRange (params.range.getBuffer()[i]);

        gate->process (channels, i, 1);
    }
}

void Gate::updateBallistics()
{
    gate->setAttack (params.attack.getCurrentValue() * 1e3f);
    gate->setHold (params.hold.getCurrentValue() * 1e3f);
    gate->setRelease (params.release.getCurrentValue() * 1e3f);
    gate->setMode (ParamUtils::choiceFromValue<dsp::Gate::Mode> (params.mode.getCurrentValue()));
}

} // namespace cgo
