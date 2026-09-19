#include <cgo_graph/cgo_graph.h>

namespace cgo
{

Compressor::Compressor()
  : Processor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true).withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
    params { .threshold = addModulatedParameter (ParamUtils::createGainParameter ("threshold", "Threshold", -60.0f, 0.0f, 0.0f)),
             .ratio = addModulatedParameter (ParamUtils::createRangedParameter (
                 "ratio",
                 "Ratio",
                 ":1",
                 ParamUtils::getRangeWithCenter (1.0f, 20.0f, 4.0f),
                 4.0f,
                 [] (float val, int) { return juce::String (val, 1); },
                 [] (const juce::String& str) { return str.getFloatValue(); })),
             .knee = addModulatedParameter (ParamUtils::createRangedParameter (
                 "knee",
                 "Knee",
                 "dB",
                 { 0.0f, 24.0f },
                 6.0f,
                 [] (float val, int) { return juce::String (val, 1); },
                 [] (const juce::String& str) { return str.getFloatValue(); })),
             .attack = addModulatedParameter (ParamUtils::createTimeParameter ("attack", "Attack", 1e-4f, 1.0f, 0.01f, 0.01f)),
             .release = addModulatedParameter (ParamUtils::createTimeParameter ("release", "Release", 1e-3f, 2.0f, 0.1f, 0.1f)),
             .makeup = addModulatedParameter (ParamUtils::createGainParameter ("makeup", "Makeup", -12.0f, 24.0f, 0.0f)),
             .mix = addModulatedParameter (ParamUtils::createPercentParameter ("mix", "Mix", 1.0f)),
             .mode = addModulatedParameter (ParamUtils::createChoiceParameter ("mode", "Mode", { "Peak", "RMS" }, 0)) }
{
}

juce::String Compressor::getTypeId() const { return "compressor"; }

void Compressor::prepareImpl() { compressor.emplace ((float) getSampleRate(), getNumChannels()); }

void Compressor::processImpl (juce::AudioBuffer<float>& buffer)
{
    if (anyChanging (params.attack, params.release, params.mode))
        updateBallistics();

    const auto channels = buffer.getArrayOfWritePointers();
    const int numSamples = buffer.getNumSamples();

    if (! anyChanging (params.threshold, params.ratio, params.knee, params.makeup, params.mix))
    {
        compressor->process (channels, 0, numSamples);
        return;
    }

    for (int i = 0; i < numSamples; i++)
    {
        if (params.threshold.isChanging())
            compressor->setThreshold (params.threshold.getBuffer()[i]);

        if (params.ratio.isChanging())
            compressor->setRatio (params.ratio.getBuffer()[i]);

        if (params.knee.isChanging())
            compressor->setKnee (params.knee.getBuffer()[i]);

        if (params.makeup.isChanging())
            compressor->setMakeup (params.makeup.getBuffer()[i]);

        if (params.mix.isChanging())
            compressor->setMix (params.mix.getBuffer()[i]);

        compressor->process (channels, i, 1);
    }
}

void Compressor::resetImpl() { compressor->reset(); }

void Compressor::updateBallistics()
{
    compressor->setAttack (params.attack.getCurrentValue() * 1e3f);
    compressor->setRelease (params.release.getCurrentValue() * 1e3f);
    compressor->setMode (ParamUtils::choiceFromValue<dsp::Compressor::Mode> (params.mode.getCurrentValue()));
}

} // namespace cgo
