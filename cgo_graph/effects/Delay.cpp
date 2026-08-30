#include <cgo_graph/cgo_graph.h>

namespace cgo
{

Delay::Delay()
  : Processor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true).withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
    params { .delayLeft = addModulatedParameter (ParamUtils::createSyncedRateParameter ("delay_l", "Delay L", "1/8")),
             .delayRight = addModulatedParameter (ParamUtils::createSyncedRateParameter ("delay_r", "Delay R", "1/8")),
             .glide = addModulatedParameter (ParamUtils::createTimeParameter ("glide", "Glide", 0.0f, 1.0f, 0.1f, 0.1f), noSmoothing),
             .mode = addModulatedParameter (ParamUtils::createChoiceParameter ("mode", "Mode", { "Normal", "Ping Pong" }, 0)),
             .feedback = addModulatedParameter (ParamUtils::createPercentParameter ("feedback", "Feedback", 0.4f)),
             .centerFrequency = addModulatedParameter (ParamUtils::createFreqParameter ("center", "Center", 20.0f, 20e3f, 1e3f, 1e3f)),
             .bandwidth = addModulatedParameter (
                 ParamUtils::createRangedParameter ("bandwidth", "Bandwidth", "oct", ParamUtils::getRangeWithCenter (0.5f, 10.0f, 3.0f), 10.0f)),
             .output = addModulatedParameter (ParamUtils::createGainParameter ("output", "Output", -30.0f, 6.0f, 0.0f)),
             .mix = addModulatedParameter (ParamUtils::createPercentParameter ("mix", "Mix", 0.35f)) }
{
}

juce::String Delay::getTypeId() const { return "delay"; }

void Delay::prepareImpl()
{
    delay.emplace ((float) getSampleRate(), getNumChannels());
    delay->reset();
}

void Delay::processImpl (juce::AudioBuffer<float>& buffer)
{
    delay->setMode (ParamUtils::choiceFromValue<dsp::Delay::Mode> (params.mode.getCurrentValue()));

    if (params.glide.isChanging())
        delay->setGlideLength (params.glide.getCurrentValue());

    updateDelayTimes();

    const auto channels = buffer.getArrayOfWritePointers();
    const int numSamples = buffer.getNumSamples();

    if (! anyChanging (params.centerFrequency, params.bandwidth, params.mix, params.output, params.feedback))
    {
        delay->process (channels, 0, numSamples);
        return;
    }

    for (int i = 0; i < numSamples; i++)
    {
        if (params.centerFrequency.isChanging() || params.bandwidth.isChanging())
            delay->setFilter (params.centerFrequency.getBuffer()[i], params.bandwidth.getBuffer()[i]);

        if (params.mix.isChanging())
            delay->setMix (params.mix.getBuffer()[i]);

        if (params.output.isChanging())
            delay->setOutputGain (juce::Decibels::decibelsToGain (params.output.getBuffer()[i]));

        delay->setFeedback (params.feedback.getBuffer()[i]);

        delay->process (channels, i, 1);
    }
}

void Delay::tempoChanged() { updateDelayTimes(); }

void Delay::updateDelayTimes()
{
    const int leftIndex = ParamUtils::choiceFromValue (params.delayLeft.getCurrentValue());
    const int rightIndex = ParamUtils::choiceFromValue (params.delayRight.getCurrentValue());

    delay->setDelayLeft ((float) (ParamUtils::syncedRateIndexToBeats (leftIndex) * getSecondsPerBeat()));
    delay->setDelayRight ((float) (ParamUtils::syncedRateIndexToBeats (rightIndex) * getSecondsPerBeat()));
}

} // namespace cgo
