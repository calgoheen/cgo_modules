#include <cgo_graph/cgo_graph.h>

namespace cgo
{

EnvelopeFollower::EnvelopeFollower()
  : params { .source = addModulatedParameter (ParamUtils::createChoiceParameter ("source", "Source", { "Input", "Sidechain" }, Source::input)),
             .attack = addModulatedParameter (ParamUtils::createTimeParameter ("attack", "Attack", 1e-4f, 1.0f, 0.01f, 5e-3f)),
             .release = addModulatedParameter (ParamUtils::createTimeParameter ("release", "Release", 1e-3f, 2.0f, 0.1f, 0.1f)),
             .gain = addModulatedParameter (ParamUtils::createGainParameter ("gain", "Gain", -24.0f, 24.0f, 0.0f)),
             .range = addModulatedParameter (ParamUtils::createRangedParameter (
                 "range",
                 "Range",
                 "dB",
                 { 6.0f, 96.0f },
                 60.0f,
                 [] (float val, int) { return juce::String (val, 1); },
                 [] (const juce::String& str) { return str.getFloatValue(); })),
             .mode = addModulatedParameter (ParamUtils::createChoiceParameter ("mode", "Mode", { "Peak", "RMS" }, 0)) }
{
}

juce::String EnvelopeFollower::getTypeId() const { return "follower"; }

void EnvelopeFollower::prepareImpl()
{
    detector.prepare ({ getSampleRate(), (juce::uint32) getBlockSize(), 1 });
    updateBallistics();
}

void EnvelopeFollower::processImpl (float* buffer, int numSamples)
{
    if (params.attack.isChanging() || params.release.isChanging() || params.mode.isChanging())
        updateBallistics();

    const bool fromSidechain = ParamUtils::choiceFromValue<Source> (params.source.getCurrentValue()) == Source::sidechain;
    const auto* source = fromSidechain ? getAudio().sidechain : getAudio().input;

    const int numChannels = source != nullptr ? source->getNumChannels() : 0;
    const float scale = numChannels > 0 ? 1.0f / (float) numChannels : 0.0f;

    const auto* gainDb = params.gain.getBuffer();
    const auto* rangeDb = params.range.getBuffer();

    for (int i = 0; i < numSamples; i++)
    {
        float sum = 0.0f;

        for (int ch = 0; ch < numChannels; ch++)
            sum += source->getReadPointer (ch)[i];

        const float level = detector.processSample (0, sum * scale);
        const float db = juce::Decibels::gainToDecibels (level, silenceDb) + gainDb[i];

        buffer[i] = juce::jlimit (0.0f, 1.0f, 1.0f + db / rangeDb[i]);
    }
}

void EnvelopeFollower::updateBallistics()
{
    const auto m = ParamUtils::choiceFromValue<Mode> (params.mode.getCurrentValue());

    if (m != mode)
    {
        mode = m;
        detector.setLevelCalculationType (m);
    }

    detector.setAttackTime (params.attack.getCurrentValue() * 1e3f);
    detector.setReleaseTime (params.release.getCurrentValue() * 1e3f);
}

} // namespace cgo
