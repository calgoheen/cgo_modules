#include <cgo_graph/cgo_graph.h>

namespace cgo
{

Chorus::Chorus()
  : Processor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true).withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
    params { .type = addModulatedParameter (ParamUtils::createChoiceParameter ("type", "Type", { "Chorus", "Ensemble" }, 0)),
             .rate = addModulatedParameter (ParamUtils::createFreqParameter ("rate", "Rate", 0.01f, 20.0f, 2.0f, 1.0f)),
             .amount = addModulatedParameter (ParamUtils::createPercentParameter ("amount", "Amount", 0.5f)),
             .feedback = addModulatedParameter (ParamUtils::createPercentParameter ("feedback", "Feedback", 0.0f)),
             .flipFeedback = addModulatedParameter (ParamUtils::createBoolParameter ("flip", "Flip", false)),
             .width = addModulatedParameter (ParamUtils::createRangedParameter (
                 "width",
                 "Width",
                 "%",
                 { 0.0f, 2.0f },
                 1.0f,
                 [] (float val, int) { return juce::String (juce::roundToInt (val * 100.0f)); },
                 [] (const juce::String& str) { return str.getFloatValue() / 100.0f; })),
             .warmth = addModulatedParameter (ParamUtils::createPercentParameter ("warmth", "Warmth", 0.0f)),
             .output = addModulatedParameter (ParamUtils::createGainParameter ("output", "Output", -30.0f, 6.0f, 0.0f)),
             .mix = addModulatedParameter (ParamUtils::createPercentParameter ("mix", "Mix", 0.5f)) }
{
}

juce::String Chorus::getTypeId() const { return "chorus"; }

void Chorus::prepareImpl() { chorus.emplace ((float) getSampleRate(), getNumChannels()); }

void Chorus::processImpl (juce::AudioBuffer<float>& buffer)
{
    if (params.type.isChanging())
        chorus->setAlgorithm (ParamUtils::choiceFromValue<dsp::Chorus::Algorithm> (params.type.getCurrentValue()));

    if (params.flipFeedback.isChanging())
        chorus->setFeedbackFlipped (ParamUtils::boolFromValue (params.flipFeedback.getCurrentValue()));

    const auto channels = buffer.getArrayOfWritePointers();
    const int numSamples = buffer.getNumSamples();

    if (! anyChanging (params.rate, params.warmth, params.mix, params.output, params.amount, params.feedback, params.width))
    {
        chorus->process (channels, 0, numSamples);
        return;
    }

    for (int i = 0; i < numSamples; i++)
    {
        if (params.rate.isChanging())
            chorus->setRate (params.rate.getBuffer()[i]);

        if (params.warmth.isChanging())
            chorus->setWarmth (params.warmth.getBuffer()[i]);

        if (params.mix.isChanging())
            chorus->setMix (params.mix.getBuffer()[i]);

        if (params.output.isChanging())
            chorus->setOutputGain (juce::Decibels::decibelsToGain (params.output.getBuffer()[i]));

        chorus->setAmount (params.amount.getBuffer()[i]);
        chorus->setFeedback (params.feedback.getBuffer()[i]);
        chorus->setWidth (params.width.getBuffer()[i]);

        chorus->process (channels, i, 1);
    }
}

void Chorus::resetImpl() { chorus->reset(); }

void Chorus::playbackStateChanged()
{
    if (getPlaybackState())
        chorus->setPhase (0.0);
}

} // namespace cgo
