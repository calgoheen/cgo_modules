#include <cgo_graph/cgo_graph.h>

namespace cgo
{

Phaser::Phaser()
  : Processor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true).withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
    params { .notches =
                 addModulatedParameter (ParamUtils::createRangedParameter ("notches", "Notches", "", { 1.0f, (float) dsp::Phaser::maxNotches, 1.0f }, 4.0f)),
             .center = addModulatedParameter (ParamUtils::createFreqParameter ("center", "Center", 70.0f, 18.5e3f, 1e3f, 1e3f)),
             .spread = addModulatedParameter (ParamUtils::createPercentParameter ("spread", "Spread", 0.5f)),
             .feedback = addModulatedParameter (ParamUtils::createPercentParameter ("feedback", "Feedback", 0.0f)),
             .flipFeedback = addModulatedParameter (ParamUtils::createBoolParameter ("flip", "Flip", false)),
             .shape = addModulatedParameter (ParamUtils::createChoiceParameter ("shape", "Shape", { "Triangle", "Sine", "Ramp Up", "Ramp Down" }, 0)),
             .rate = addModulatedParameter (ParamUtils::createSyncedRateParameter ("rate", "Rate")),
             .amount = addModulatedParameter (ParamUtils::createPercentParameter ("amount", "Amount", 1.0f)),
             .blend = addModulatedParameter (ParamUtils::createPercentParameter ("blend", "Blend", 0.0f)),
             .offset = addModulatedParameter (ParamUtils::createPercentParameter ("offset", "Offset", 0.5f)),
             .safeBass = addModulatedParameter (ParamUtils::createFreqParameter ("safe_bass", "Safe Bass", 10.0f, 3000.0f, 500.0f, 100.0f)),
             .warmth = addModulatedParameter (ParamUtils::createPercentParameter ("warmth", "Warmth", 0.0f)),
             .output = addModulatedParameter (ParamUtils::createGainParameter ("output", "Output", -30.0f, 6.0f, 0.0f)),
             .mix = addModulatedParameter (ParamUtils::createPercentParameter ("mix", "Mix", 1.0f)) }
{
}

juce::String Phaser::getTypeId() const { return "phaser"; }

void Phaser::prepareImpl() { phaser.emplace ((float) getSampleRate(), getNumChannels()); }

void Phaser::processImpl (juce::AudioBuffer<float>& buffer)
{
    if (params.shape.isChanging())
        phaser->setShape (ParamUtils::choiceFromValue<dsp::LfoShape::Shape> (params.shape.getCurrentValue()));

    if (params.flipFeedback.isChanging())
        phaser->setFeedbackFlipped (ParamUtils::boolFromValue (params.flipFeedback.getCurrentValue()));

    if (params.notches.isChanging())
        phaser->setNotches (ParamUtils::choiceFromValue (params.notches.getCurrentValue()));

    if (params.rate.isChanging())
        updateRateSync();

    const auto channels = buffer.getArrayOfWritePointers();
    const int numSamples = buffer.getNumSamples();

    if (! anyChanging (params.safeBass,
                       params.warmth,
                       params.mix,
                       params.output,
                       params.center,
                       params.spread,
                       params.feedback,
                       params.amount,
                       params.blend,
                       params.offset))
    {
        phaser->process (channels, 0, numSamples);
        return;
    }

    for (int i = 0; i < numSamples; i++)
    {
        if (params.safeBass.isChanging())
            phaser->setSafeBass (params.safeBass.getBuffer()[i]);

        if (params.warmth.isChanging())
            phaser->setWarmth (params.warmth.getBuffer()[i]);

        if (params.mix.isChanging())
            phaser->setMix (params.mix.getBuffer()[i]);

        if (params.output.isChanging())
            phaser->setOutputGain (juce::Decibels::decibelsToGain (params.output.getBuffer()[i]));

        if (params.center.isChanging())
            phaser->setCenter (params.center.getBuffer()[i]);

        if (params.spread.isChanging())
            phaser->setSpread (params.spread.getBuffer()[i]);

        phaser->setFeedback (params.feedback.getBuffer()[i]);
        phaser->setAmount (params.amount.getBuffer()[i]);
        phaser->setBlend (params.blend.getBuffer()[i]);
        phaser->setStereoPhaseOffset (params.offset.getBuffer()[i]);

        phaser->process (channels, i, 1);
    }
}

void Phaser::resetImpl() { phaser->reset(); }

void Phaser::playbackStateChanged()
{
    if (getPlaybackState())
        updateRateSync();
}

void Phaser::tempoChanged() { updateRateSync(); }

void Phaser::playHeadJumped() { updateRateSync(); }

void Phaser::updateRateSync()
{
    const int idx = ParamUtils::choiceFromValue (params.rate.getCurrentValue());
    const double periodInBeats = ParamUtils::syncedRateIndexToBeats (idx);

    phaser->setRate ((float) (1.0 / (periodInBeats * getSecondsPerBeat())));

    if (getPlaybackState())
        phaser->setPhase (std::fmod (getPlaybackPosition(), periodInBeats) / periodInBeats);
}

} // namespace cgo
