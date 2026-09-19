#include <cgo_graph/cgo_graph.h>

namespace cgo
{

Flanger::Flanger()
  : Processor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true).withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
    params { .delay = addModulatedParameter (
                 ParamUtils::createTimeParameter ("delay", "Delay Time", dsp::Flanger::minDelaySeconds, dsp::Flanger::maxDelaySeconds, 2e-3f, 2.5e-3f)),
             .feedback = addModulatedParameter (ParamUtils::createPercentParameter ("feedback", "Feedback", 0.0f)),
             .flipFeedback = addModulatedParameter (ParamUtils::createBoolParameter ("flip", "Flip", false)),
             .shape = addModulatedParameter (ParamUtils::createChoiceParameter ("shape", "Shape", { "Triangle", "Sine", "Ramp Up", "Ramp Down" }, 0)),
             .rate = addModulatedParameter (ParamUtils::createSyncedRateParameter ("rate", "Rate")),
             .depth = addModulatedParameter (ParamUtils::createPercentParameter ("depth", "Depth", 1.0f)),
             .offset = addModulatedParameter (ParamUtils::createPercentParameter ("offset", "Offset", 0.5f)),
             .safeBass = addModulatedParameter (ParamUtils::createFreqParameter ("safe_bass", "Safe Bass", 10.0f, 3000.0f, 500.0f, 100.0f)),
             .warmth = addModulatedParameter (ParamUtils::createPercentParameter ("warmth", "Warmth", 0.0f)),
             .output = addModulatedParameter (ParamUtils::createGainParameter ("output", "Output", -30.0f, 6.0f, 0.0f)),
             .mix = addModulatedParameter (ParamUtils::createPercentParameter ("mix", "Mix", 1.0f)) }
{
}

juce::String Flanger::getTypeId() const { return "flanger"; }

void Flanger::prepareImpl() { flanger.emplace ((float) getSampleRate(), getNumChannels()); }

void Flanger::processImpl (juce::AudioBuffer<float>& buffer)
{
    if (params.shape.isChanging())
        flanger->setShape (ParamUtils::choiceFromValue<dsp::LfoShape::Shape> (params.shape.getCurrentValue()));

    if (params.flipFeedback.isChanging())
        flanger->setFeedbackFlipped (ParamUtils::boolFromValue (params.flipFeedback.getCurrentValue()));

    if (params.rate.isChanging())
        updateRateSync();

    const auto channels = buffer.getArrayOfWritePointers();
    const int numSamples = buffer.getNumSamples();

    if (! anyChanging (params.safeBass, params.warmth, params.mix, params.output, params.delay, params.feedback, params.depth, params.offset))
    {
        flanger->process (channels, 0, numSamples);
        return;
    }

    for (int i = 0; i < numSamples; i++)
    {
        if (params.safeBass.isChanging())
            flanger->setSafeBass (params.safeBass.getBuffer()[i]);

        if (params.warmth.isChanging())
            flanger->setWarmth (params.warmth.getBuffer()[i]);

        if (params.mix.isChanging())
            flanger->setMix (params.mix.getBuffer()[i]);

        if (params.output.isChanging())
            flanger->setOutputGain (juce::Decibels::decibelsToGain (params.output.getBuffer()[i]));

        flanger->setDelay (params.delay.getBuffer()[i]);
        flanger->setFeedback (params.feedback.getBuffer()[i]);
        flanger->setDepth (params.depth.getBuffer()[i]);
        flanger->setStereoPhaseOffset (params.offset.getBuffer()[i]);

        flanger->process (channels, i, 1);
    }
}

void Flanger::resetImpl() { flanger->reset(); }

void Flanger::playbackStateChanged()
{
    if (getPlaybackState())
        updateRateSync();
}

void Flanger::tempoChanged() { updateRateSync(); }

void Flanger::playHeadJumped() { updateRateSync(); }

void Flanger::updateRateSync()
{
    const int idx = ParamUtils::choiceFromValue (params.rate.getCurrentValue());
    const double periodInBeats = ParamUtils::syncedRateIndexToBeats (idx);

    flanger->setRate ((float) (1.0 / (periodInBeats * getSecondsPerBeat())));

    if (getPlaybackState())
        flanger->setPhase (std::fmod (getPlaybackPosition(), periodInBeats) / periodInBeats);
}

} // namespace cgo
