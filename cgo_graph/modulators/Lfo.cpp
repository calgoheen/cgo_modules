#include <cgo_graph/cgo_graph.h>

namespace cgo
{

Lfo::Lfo()
  : params { .shape = addModulatedParameter (
                 ParamUtils::createChoiceParameter ("shape", "Shape", { "Triangle", "Sine", "Ramp Up", "Ramp Down" }, dsp::LfoShape::sine)),
             .sync = addModulatedParameter (ParamUtils::createBoolParameter ("sync", "Sync", true)),
             .rateFree = addModulatedParameter (ParamUtils::createFreqParameter ("rate_free", "Rate Free", 0.1f, 100.0f, 10.0f, 1.0f)),
             .rateSync = addModulatedParameter (ParamUtils::createSyncedRateParameter ("rate_sync", "Rate Sync")) }
{
}

juce::String Lfo::getTypeId() const { return "lfo"; }

void Lfo::prepareImpl()
{
    dsp::LfoShape::init();

    phasor.setPhase (0.0);
}

void Lfo::processImpl (float* buffer, int numSamples)
{
    const auto shape = ParamUtils::choiceFromValue<dsp::LfoShape::Shape> (params.shape.getCurrentValue());
    const bool syncOn = ParamUtils::boolFromValue (params.sync.getCurrentValue());

    if (syncOn && (params.sync.isChanging() || params.rateSync.isChanging()))
        updateRateSync();
    else if (! syncOn && params.sync.isChanging())
        phasor.setFrequency (params.rateFree.getCurrentValue(), getSampleRate());

    for (int i = 0; i < numSamples; i++)
    {
        if (! syncOn && params.rateFree.isChanging())
            phasor.setFrequency (params.rateFree.getBuffer()[i], getSampleRate());

        buffer[i] = dsp::LfoShape::get (shape, phasor.getFloatAndInc(), (float) phasor.getFrequency());
    }
}

void Lfo::playbackStateChanged()
{
    if (getPlaybackState())
    {
        if (ParamUtils::boolFromValue (params.sync.getCurrentValue()))
            updateRateSync();
        else
            phasor.setPhase (0.0);
    }
}

void Lfo::tempoChanged()
{
    if (ParamUtils::boolFromValue (params.sync.getCurrentValue()))
        updateRateSync();
}

void Lfo::playHeadJumped()
{
    if (ParamUtils::boolFromValue (params.sync.getCurrentValue()))
        updateRateSync();
}

void Lfo::updateRateSync()
{
    const int idx = ParamUtils::choiceFromValue (params.rateSync.getCurrentValue());
    const double periodInBeats = ParamUtils::syncedRateIndexToBeats (idx);

    phasor.setFrequency (1.0 / (periodInBeats * getSecondsPerBeat()), getSampleRate());

    if (getPlaybackState())
        phasor.setPhase (std::fmod (getPlaybackPosition(), periodInBeats) / periodInBeats);
}

} // namespace cgo
