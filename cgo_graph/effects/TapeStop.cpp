#include <cgo_graph/cgo_graph.h>

namespace cgo
{

CGO_ANON_NAMESPACE_BEGIN

constexpr float rampStart = 0.0f;
constexpr float rampEnd = 1.0f;
constexpr float fadeLengthSeconds = 0.02f;
constexpr float crossfadeLengthSeconds = 0.02f;

CGO_ANON_NAMESPACE_END

TapeStop::TapeStop()
  : Processor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true).withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
    params { .mode = addModulatedParameter (ParamUtils::createChoiceParameter ("mode", "Mode", { "Bypass", "Stop", "Start" }, 0)),
             .length = addModulatedParameter (ParamUtils::createSyncedRateParameter ("length", "Length")),
             .curve = addModulatedParameter (ParamUtils::createRangedParameter ("curve", "Curve", "", { -1.0f, 1.0f }, 0.0f), noSmoothing),
             .autoBypass = addModulatedParameter (ParamUtils::createBoolParameter ("auto_byp", "Auto Bypass", false)) }
{
}

juce::String TapeStop::getTypeId() const { return "tapestop"; }

void TapeStop::prepareImpl()
{
    tapeStop.emplace ((float) getSampleRate(), getNumChannels());

    tapeStop->setSlowdownRange (ANON::rampStart, ANON::rampEnd);
    tapeStop->setSpeedupRange (ANON::rampStart, ANON::rampEnd);
    tapeStop->setFadeLength (ANON::fadeLengthSeconds);
    tapeStop->setCrossfadeLength (ANON::crossfadeLengthSeconds);

    lastObservedMode = ParamUtils::choiceFromValue<dsp::TapeStop::Mode> (params.mode.getCurrentValue());

    updateParameters();
    tapeStop->setMode (lastObservedMode, true);
}

void TapeStop::processImpl (juce::AudioBuffer<float>& audioBuffer)
{
    updateParameters();

    tapeStop->process (audioBuffer.getArrayOfWritePointers(), 0, audioBuffer.getNumSamples());
}

void TapeStop::tempoChanged() { updateLength(); }

void TapeStop::updateParameters()
{
    tapeStop->setAutoBypass (ParamUtils::boolFromValue (params.autoBypass.getCurrentValue()));

    updateLength();

    const float curve = params.curve.getCurrentValue();
    tapeStop->setSlowdownCurve (curve);
    tapeStop->setSpeedupCurve (curve);

    const auto observedMode = ParamUtils::choiceFromValue<dsp::TapeStop::Mode> (params.mode.getCurrentValue());
    const bool controlMoved = observedMode != lastObservedMode || params.mode.wasTouched();
    lastObservedMode = observedMode;

    if (controlMoved && tapeStop->getMode() != observedMode)
        tapeStop->setMode (observedMode);
}

void TapeStop::updateLength()
{
    const int lengthIndex = ParamUtils::choiceFromValue (params.length.getCurrentValue());
    const auto lengthSeconds = (float) (ParamUtils::syncedRateIndexToBeats (lengthIndex) * getSecondsPerBeat());

    tapeStop->setSlowdownLength (lengthSeconds);
    tapeStop->setSpeedupLength (lengthSeconds);
}

} // namespace cgo
