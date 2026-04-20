#include <cgo_processors/cgo_processors.h>

namespace cgo
{

Reverb::Reverb (Params& params) 
    : parameters (params)
{
    addParameterUpdaters (*parameters.predelay,
                          [this] (float val)
                          {
                              predelaySmoother.setTargetValue (val);
                          },
                          *parameters.size,
                          [this] (float val)
                          {
                              sizeSmoother.setTargetValue (val);
                          },
                          *parameters.decay,
                          [this] (float val)
                          {
                              decaySmoother.setTargetValue (val);
                          },
                          *parameters.inputFilterLowpassCutoff,
                          [this] (float val)
                          {
                              iflpCutoffSmoother.setTargetValue (val);
                          },
                          *parameters.inputFilterHighpassCutoff,
                          [this] (float val)
                          {
                              ifhpCutoffSmoother.setTargetValue (val);
                          },
                          *parameters.diffusionModRate,
                          [this] (float val)
                          {
                              diffModRateSmoother.setTargetValue (val);
                          },
                          *parameters.diffusionModDepth,
                          [this] (float val)
                          {
                              diffModDepthSmoother.setTargetValue (val);
                          },
                          *parameters.tailFilterLowCutoff,
                          [this] (float val)
                          {
                              tlsCutoffSmoother.setTargetValue (val);
                          },
                          *parameters.tailFilterLowGain,
                          [this] (float val)
                          {
                              tlsGainSmoother.setTargetValue (val);
                          },
                          *parameters.tailFilterHighCutoff,
                          [this] (float val)
                          {
                              thsCutoffSmoother.setTargetValue (val);
                          },
                          *parameters.tailFilterHighGain,
                          [this] (float val)
                          {
                              thsGainSmoother.setTargetValue (val);
                          },
                          *parameters.tailModRate,
                          [this] (float val)
                          {
                              tailModRateSmoother.setTargetValue (val);
                          },
                          *parameters.tailModDepth,
                          [this] (float val)
                          {
                              tailModDepthSmoother.setTargetValue (val);
                          },
                          *parameters.earlyReflectionGain,
                          [this] (float val)
                          {
                              earlyReflectionGainSmoother.setTargetValue (juce::Decibels::decibelsToGain (val));
                          },
                          *parameters.diffusionGain,
                          [this] (float val)
                          {
                              diffusionGainSmoother.setTargetValue (juce::Decibels::decibelsToGain (val));
                          },
                          *parameters.mix,
                          [this] (float val)
                          {
                              mixSmoother.setTargetValue (val);
                          });

    LfoTable::initLinear<numLfoPoints>();
}

void Reverb::reset()
{
    jassert (getNumChannels() == 2);
    diffusionReverb.emplace (maxSizeSeconds, getSampleRate());

    sizeSmoother.reset (getSampleRate(), smoothingTimeSeconds);
    decaySmoother.reset (getSampleRate(), smoothingTimeSeconds);
    iflpCutoffSmoother.reset (getSampleRate(), smoothingTimeSeconds);
    ifhpCutoffSmoother.reset (getSampleRate(), smoothingTimeSeconds);
    tlsCutoffSmoother.reset (getSampleRate(), smoothingTimeSeconds);
    tlsGainSmoother.reset (getSampleRate(), smoothingTimeSeconds);
    thsCutoffSmoother.reset (getSampleRate(), smoothingTimeSeconds);
    thsGainSmoother.reset (getSampleRate(), smoothingTimeSeconds);
    diffModRateSmoother.reset (getSampleRate(), smoothingTimeSeconds);
    diffModDepthSmoother.reset (getSampleRate(), smoothingTimeSeconds);
    tailModRateSmoother.reset (getSampleRate(), smoothingTimeSeconds);
    tailModDepthSmoother.reset (getSampleRate(), smoothingTimeSeconds);
    diffusionGainSmoother.reset (getSampleRate(), smoothingTimeSeconds);
    earlyReflectionGainSmoother.reset (getSampleRate(), smoothingTimeSeconds);
    predelaySmoother.reset (getSampleRate(), smoothingTimeSeconds);
    mixSmoother.reset (getSampleRate(), smoothingTimeSeconds);

    diffusionReverb->setSize (sizeSmoother.getCurrentValue());
    diffusionReverb->setDecay (decaySmoother.getCurrentValue());
    diffusionReverb->setInputFilter (DiffusionReverbFilterType::lowpass, iflpCutoffSmoother.getCurrentValue());
    diffusionReverb->setInputFilter (DiffusionReverbFilterType::highpass, ifhpCutoffSmoother.getCurrentValue());
    diffusionReverb->setDecayFilter (MultiTapDelay::FilterType::lowShelf, tlsGainSmoother.getCurrentValue(), tlsCutoffSmoother.getCurrentValue());
    diffusionReverb->setDecayFilter (MultiTapDelay::FilterType::highShelf, thsGainSmoother.getCurrentValue(), thsCutoffSmoother.getCurrentValue());
    diffusionReverb->setDiffusionModRate (diffModRateSmoother.getCurrentValue());
    diffusionReverb->setDiffusionModDepth (diffModDepthSmoother.getCurrentValue());
    diffusionReverb->setTailModRate (tailModRateSmoother.getCurrentValue());
    diffusionReverb->setTailModDepth (tailModDepthSmoother.getCurrentValue());
    diffusionReverb->setEarlyReflectionGain (earlyReflectionGainSmoother.getCurrentValue());
    diffusionReverb->setDiffusionGain (diffusionGainSmoother.getCurrentValue());
    diffusionReverb->setPredelay (predelaySmoother.getCurrentValue());
}

void Reverb::process (float* const* buffer, int startIndex, int numSamples)
{
    const int endIndex = startIndex + numSamples;

    for (int i = startIndex; i < endIndex; i++)
    {
        if (sizeSmoother.isSmoothing())
            diffusionReverb->setSize (sizeSmoother.getNextValue());

        if (decaySmoother.isSmoothing())
            diffusionReverb->setDecay (decaySmoother.getNextValue());

        if (iflpCutoffSmoother.isSmoothing())
            diffusionReverb->setInputFilter (DiffusionReverbFilterType::lowpass, iflpCutoffSmoother.getNextValue());

        if (ifhpCutoffSmoother.isSmoothing())
            diffusionReverb->setInputFilter (DiffusionReverbFilterType::highpass, ifhpCutoffSmoother.getNextValue());

        if (tlsCutoffSmoother.isSmoothing() || tlsGainSmoother.isSmoothing())
            diffusionReverb->setDecayFilter (MultiTapDelay::FilterType::lowShelf, tlsGainSmoother.getNextValue(), tlsCutoffSmoother.getNextValue());

        if (thsCutoffSmoother.isSmoothing() || thsGainSmoother.isSmoothing())
            diffusionReverb->setDecayFilter (MultiTapDelay::FilterType::highShelf, thsGainSmoother.getNextValue(), thsCutoffSmoother.getNextValue());

        if (diffModRateSmoother.isSmoothing())
            diffusionReverb->setDiffusionModRate (diffModRateSmoother.getNextValue());

        if (diffModDepthSmoother.isSmoothing())
            diffusionReverb->setDiffusionModDepth (diffModDepthSmoother.getNextValue());

        if (tailModRateSmoother.isSmoothing())
            diffusionReverb->setTailModRate (tailModRateSmoother.getNextValue());

        if (tailModDepthSmoother.isSmoothing())
            diffusionReverb->setTailModDepth (tailModDepthSmoother.getNextValue());

        if (earlyReflectionGainSmoother.isSmoothing())
            diffusionReverb->setEarlyReflectionGain (earlyReflectionGainSmoother.getNextValue());

        if (diffusionGainSmoother.isSmoothing())
            diffusionReverb->setDiffusionGain (diffusionGainSmoother.getNextValue());

        if (predelaySmoother.isSmoothing())
            diffusionReverb->setPredelay (predelaySmoother.getNextValue());

        const float mix = mixSmoother.getNextValue();

        auto dry = std::array { buffer[0][i], buffer[1][i] };
        auto wet = dry;
        diffusionReverb->process (wet[0], wet[1]);

        const auto [dryGain, wetGain] = AudioUtils::constantPowerMix (mix);
        buffer[0][i] = dry[0] * dryGain + wet[0] * wetGain;
        buffer[1][i] = dry[1] * dryGain + wet[1] * wetGain;
    }
}

} // namespace cgo
