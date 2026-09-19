#include <cgo_graph/cgo_graph.h>

namespace cgo
{

CGO_ANON_NAMESPACE_BEGIN

constexpr float curveScale = 0.5f;

CGO_ANON_NAMESPACE_END

Lfo::Lfo()
  : params { .shape = addModulatedParameter (
                 ParamUtils::createChoiceParameter ("shape", "Shape", { "Triangle", "Sine", "Square", "Ramp Up", "Ramp Down" }, dsp::LfoShape::sine)),
             .sync = addModulatedParameter (ParamUtils::createBoolParameter ("sync", "Sync", true)),
             .rateFree = addModulatedParameter (ParamUtils::createFreqParameter ("rate_free", "Rate Free", 0.1f, 100.0f, 10.0f, 1.0f)),
             .rateSync = addModulatedParameter (ParamUtils::createSyncedRateParameter ("rate_sync", "Rate Sync", "1/4")),
             .phase = addModulatedParameter (ParamUtils::createPercentParameter ("phase", "Phase", 0.0f)),
             .curve = addModulatedParameter (ParamUtils::createRangedParameter ("curve", "Curve", "", { -1.0f, 1.0f }, 0.0f)) }
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

    const auto* phase = params.phase.getBuffer();
    const auto* curve = params.curve.getBuffer();

    for (int i = 0; i < numSamples; i++)
    {
        if (! syncOn && params.rateFree.isChanging())
            phasor.setFrequency (params.rateFree.getBuffer()[i], getSampleRate());

        const float value = dsp::LfoShape::get (shape, phasor.getFloat (phase[i]), (float) phasor.getFrequency());
        buffer[i] = dsp::Curve::exponential (value, ANON::curveScale * -curve[i]);

        phasor.inc();
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
