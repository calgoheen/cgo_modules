#include <cgo_graph/cgo_graph.h>

namespace cgo
{

Random::Random()
  : params { .sync = addModulatedParameter (ParamUtils::createBoolParameter ("sync", "Sync", true)),
             .rateFree =
                 addModulatedParameter (ParamUtils::createFreqParameter ("rate_free", "Rate Free", 0.1f, 100.0f, 10.0f, 1.0f), {}, ModulatedValue::Rate::audio),
             .rateSync = addModulatedParameter (ParamUtils::createSyncedRateParameter ("rate_sync", "Rate Sync", "1/4")),
             .smooth = addModulatedParameter (ParamUtils::createPercentParameter ("smooth", "Smooth", 0.0f)),
             .steps = addModulatedParameter (ParamUtils::createRangedParameter (
                                                 "steps",
                                                 "Steps",
                                                 "",
                                                 { 1.0f, 32.0f, 1.0f },
                                                 1.0f,
                                                 [] (float x, int) { return x < 2.0f ? juce::String ("Off") : juce::String (juce::roundToInt (x)); },
                                                 [] (const juce::String& str) { return str.getFloatValue(); }),
                                             noSmoothing) }
{
}

juce::String Random::getTypeId() const { return "random"; }

void Random::prepareImpl()
{
    phasor.setPhase (0.0);
    lastPhase = 0.0f;

    step();
    previousValue = currentValue;
}

void Random::processImpl (float* buffer, int numSamples)
{
    const bool syncOn = ParamUtils::boolFromValue (params.sync.getCurrentValue());

    if (syncOn && (params.sync.isChanging() || params.rateSync.isChanging()))
        updateRateSync();
    else if (! syncOn && params.sync.isChanging())
        phasor.setFrequency (params.rateFree.getCurrentValue(), getSampleRate());

    const auto* smooth = params.smooth.getBuffer();

    for (int i = 0; i < numSamples; i++)
    {
        if (! syncOn && params.rateFree.isChanging())
            phasor.setFrequency (params.rateFree.getBuffer()[i], getSampleRate());

        const float phase = phasor.getFloatAndInc();

        if (phase < lastPhase)
            step();

        lastPhase = phase;

        // The glide occupies the first `smooth` of a step, so every draw is reached whatever the setting
        const float t = smooth[i] > 0.0f ? juce::jmin (phase / smooth[i], 1.0f) : 1.0f;
        buffer[i] = previousValue + (currentValue - previousValue) * t * t * (3.0f - 2.0f * t);
    }
}

void Random::playbackStateChanged()
{
    if (! getPlaybackState())
        return;

    if (ParamUtils::boolFromValue (params.sync.getCurrentValue()))
        updateRateSync();
    else
        phasor.setPhase (0.0);

    lastPhase = phasor.getFloat();

    step();
    previousValue = currentValue;
}

void Random::tempoChanged()
{
    if (ParamUtils::boolFromValue (params.sync.getCurrentValue()))
        updateRateSync();
}

void Random::playHeadJumped()
{
    if (ParamUtils::boolFromValue (params.sync.getCurrentValue()))
        updateRateSync();
}

void Random::updateRateSync()
{
    const int idx = ParamUtils::choiceFromValue (params.rateSync.getCurrentValue());
    const double periodInBeats = ParamUtils::syncedRateIndexToBeats (idx);

    phasor.setFrequency (1.0 / (periodInBeats * getSecondsPerBeat()), getSampleRate());

    if (getPlaybackState())
    {
        phasor.setPhase (std::fmod (getPlaybackPosition(), periodInBeats) / periodInBeats);
        lastPhase = phasor.getFloat();
    }
}

void Random::step()
{
    const int numSteps = juce::roundToInt (params.steps.getCurrentValue());
    const float value = rng.nextFloat();

    previousValue = currentValue;
    currentValue = numSteps > 1 ? std::round (value * (float) (numSteps - 1)) / (float) (numSteps - 1) : value;
}

} // namespace cgo
