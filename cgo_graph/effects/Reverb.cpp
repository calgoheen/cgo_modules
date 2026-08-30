#include <cgo_graph/cgo_graph.h>

namespace cgo
{

CGO_ANON_NAMESPACE_BEGIN

constexpr float maxSizeSeconds = 0.2f;

constexpr float minDiffusionModRate = 0.25f;
constexpr float maxDiffusionModRate = 1.5f;
constexpr float minTailModRate = 0.02f;
constexpr float maxTailModRate = 0.17f;

constexpr float fullDampingGainDb = -12.0f;
constexpr float lowShelfCutoffAtNoDamping = 100.0f;
constexpr float lowShelfCutoffAtFullDamping = 700.0f;
constexpr float highShelfCutoffAtNoDamping = 3e3f;
constexpr float highShelfCutoffAtFullDamping = 1e3f;

juce::String stringFromBlend (float val, int)
{
    const int amount = juce::roundToInt (std::abs (val) * 100.0f);

    if (amount == 0)
        return "C";

    return juce::String (val < 0.0f ? "E" : "T") + juce::String (amount);
}

float blendFromString (const juce::String& str)
{
    const auto trimmed = str.trim();

    if (trimmed.startsWithIgnoreCase ("E"))
        return -trimmed.substring (1).getFloatValue() / 100.0f;

    if (trimmed.startsWithIgnoreCase ("T"))
        return trimmed.substring (1).getFloatValue() / 100.0f;

    return trimmed.getFloatValue() / 100.0f;
}

CGO_ANON_NAMESPACE_END

Reverb::Reverb()
  : Processor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true).withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
    params { .predelay = addModulatedParameter (ParamUtils::createTimeParameter ("predelay", "Predelay", 0.0005f, 0.25f, 0.05f, 0.0005f)),
             .size = addModulatedParameter (ParamUtils::createTimeParameter ("size", "Size", 0.01f, ANON::maxSizeSeconds, 0.1f, 0.1f)),
             .decay = addModulatedParameter (ParamUtils::createTimeParameter ("decay", "Decay", 0.01f, 20.0f, 2.0f, 2.0f)),
             .inputCutoff = addModulatedParameter (ParamUtils::createFreqParameter ("in_cut", "Input Cutoff", 20.0f, 20e3f, 1e3f, 1e3f)),
             .inputBandwidth = addModulatedParameter (
                 ParamUtils::createRangedParameter ("in_bw", "Input Bandwidth", "oct", ParamUtils::getRangeWithCenter (0.5f, 10.0f, 3.0f), 10.0f)),
             .modulation = addModulatedParameter (ParamUtils::createPercentParameter ("mod", "Modulation", 0.2f)),
             .lowDamping = addModulatedParameter (ParamUtils::createPercentParameter ("low_damp", "Low Damping", 0.25f)),
             .highDamping = addModulatedParameter (ParamUtils::createPercentParameter ("high_damp", "High Damping", 0.25f)),
             .blend = addModulatedParameter (
                 ParamUtils::createRangedParameter ("blend", "Blend", "", { -1.0f, 1.0f }, 0.0f, ANON::stringFromBlend, ANON::blendFromString)),
             .mix = addModulatedParameter (ParamUtils::createPercentParameter ("mix", "Mix", 0.5f)) }
{
}

juce::String Reverb::getTypeId() const { return "reverb"; }

bool Reverb::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannels() == 2 && Processor::isBusesLayoutSupported (layouts);
}

void Reverb::prepareImpl() { diffusionReverb.emplace ((float) getSampleRate(), getNumChannels(), ANON::maxSizeSeconds); }

void Reverb::processImpl (juce::AudioBuffer<float>& buffer)
{
    const auto channels = buffer.getArrayOfWritePointers();
    const int numSamples = buffer.getNumSamples();

    if (! anyChanging (params.predelay,
                       params.size,
                       params.decay,
                       params.inputCutoff,
                       params.inputBandwidth,
                       params.modulation,
                       params.lowDamping,
                       params.highDamping,
                       params.blend,
                       params.mix))
    {
        diffusionReverb->process (channels, 0, numSamples);
        return;
    }

    for (int i = 0; i < numSamples; i++)
    {
        if (params.size.isChanging())
            diffusionReverb->setSize (params.size.getBuffer()[i]);

        if (params.decay.isChanging())
            diffusionReverb->setDecay (params.decay.getBuffer()[i]);

        if (params.inputCutoff.isChanging() || params.inputBandwidth.isChanging())
            diffusionReverb->setInputFilter (params.inputCutoff.getBuffer()[i], params.inputBandwidth.getBuffer()[i]);

        if (params.modulation.isChanging())
        {
            const float amount = params.modulation.getBuffer()[i];

            diffusionReverb->setDiffusionModRate (juce::jmap (amount, ANON::minDiffusionModRate, ANON::maxDiffusionModRate));
            diffusionReverb->setDiffusionModDepth (amount);
            diffusionReverb->setTailModRate (juce::jmap (amount, ANON::minTailModRate, ANON::maxTailModRate));
            diffusionReverb->setTailModDepth (amount);
        }

        if (params.lowDamping.isChanging())
        {
            const float amount = params.lowDamping.getBuffer()[i];

            diffusionReverb->setDecayFilter (dsp::DiffusionReverbDecayFilterType::lowShelf,
                                             amount * ANON::fullDampingGainDb,
                                             juce::jmap (amount, ANON::lowShelfCutoffAtNoDamping, ANON::lowShelfCutoffAtFullDamping));
        }

        if (params.highDamping.isChanging())
        {
            const float amount = params.highDamping.getBuffer()[i];

            diffusionReverb->setDecayFilter (dsp::DiffusionReverbDecayFilterType::highShelf,
                                             amount * ANON::fullDampingGainDb,
                                             juce::jmap (amount, ANON::highShelfCutoffAtNoDamping, ANON::highShelfCutoffAtFullDamping));
        }

        if (params.blend.isChanging())
        {
            const auto [early, tail] = dsp::AudioUtils::constantPowerMix (0.5f * (params.blend.getBuffer()[i] + 1.0f));

            diffusionReverb->setEarlyReflectionGain (early);
            diffusionReverb->setDiffusionGain (tail);
        }

        if (params.predelay.isChanging())
            diffusionReverb->setPredelay (params.predelay.getBuffer()[i]);

        if (params.mix.isChanging())
            diffusionReverb->setMix (params.mix.getBuffer()[i]);

        diffusionReverb->process (channels, i, 1);
    }
}

} // namespace cgo
