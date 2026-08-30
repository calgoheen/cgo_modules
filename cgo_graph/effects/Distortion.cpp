#include <cgo_graph/cgo_graph.h>

namespace cgo
{

Distortion::Distortion()
  : Processor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true).withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
    params { .drive = addModulatedParameter (ParamUtils::createGainParameter ("drive", "Drive", 0.0f, 48.0f, 0.0f)),
             .type = addModulatedParameter (ParamUtils::createChoiceParameter ("type", "Type", { "Soft", "Hard", "Tanh", "Sine", "Fold" }, 0)),
             .output = addModulatedParameter (ParamUtils::createGainParameter ("output", "Output", -30.0f, 6.0f, 0.0f)),
             .mix = addModulatedParameter (ParamUtils::createPercentParameter ("mix", "Mix", 1.0f)) }
{
}

juce::String Distortion::getTypeId() const { return "distortion"; }

void Distortion::prepareImpl()
{
    distortion.emplace (getNumChannels());

    setLatencySamples (dsp::Distortion::latencySamples);
}

void Distortion::processImpl (juce::AudioBuffer<float>& buffer)
{
    if (params.type.isChanging())
        distortion->setType (ParamUtils::choiceFromValue<dsp::Distortion::Type> (params.type.getCurrentValue()));

    const auto channels = buffer.getArrayOfWritePointers();
    const int numSamples = buffer.getNumSamples();

    if (! anyChanging (params.drive, params.output, params.mix))
    {
        distortion->process (channels, 0, numSamples);
        return;
    }

    for (int i = 0; i < numSamples; i++)
    {
        if (params.drive.isChanging())
            distortion->setDrive (juce::Decibels::decibelsToGain (params.drive.getBuffer()[i]));

        if (params.output.isChanging())
            distortion->setOutputGain (juce::Decibels::decibelsToGain (params.output.getBuffer()[i]));

        if (params.mix.isChanging())
            distortion->setMix (params.mix.getBuffer()[i]);

        distortion->process (channels, i, 1);
    }
}

} // namespace cgo
