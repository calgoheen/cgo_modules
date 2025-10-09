#include <cgo_processors/cgo_processors.h>

namespace cgo
{

namespace
{
    constexpr float cutoffRangeStart = 20.0f;
    constexpr float cutoffRangeEnd = 22e3f;
    constexpr float resonanceRangeStart = 3e-3f;
    constexpr float resonanceRangeEnd = 15.0f;
    constexpr float modAmplitude = 0.4f;
}

Phaser::Phaser (Params& params)
    : parameters (params)
{
    addParameterUpdaters (*parameters.notches,
                          [this] (float val)
                          {
                              notches = juce::roundToInt (val);
                          },
                          *parameters.center,
                          [this] (float val)
                          {
                              centerSmoother.setTargetValue (juce::mapFromLog10 (val, cutoffRangeStart, cutoffRangeEnd));
                          },
                          *parameters.spread,
                          [this] (float val)
                          {
                              static constexpr float resonanceParamRangeStart = 0.15f;
                              static constexpr float resonanceParamRangeEnd = 2.25f;

                              const float resonance = juce::mapToLog10 (1.0f - val, resonanceParamRangeStart, resonanceParamRangeEnd);
                              const float resonanceNormalized = juce::mapFromLog10 (resonance, resonanceRangeStart, resonanceRangeEnd);
                              spreadSmoother.setTargetValue (resonanceNormalized);
                          },
                          *parameters.feedback,
                          [this] (float val)
                          {
                              feedbackSmoother.setTargetValue (0.99f * val);
                          },
                          *parameters.flipFeedback,
                          [this] (float val)
                          {
                              feedbackSign = val == 0.0f ? 1.0f : -1.0f;
                          },
                          *parameters.shape,
                          [this] (float val)
                          {
                              shape = static_cast<LfoTable::Shape> (juce::roundToInt (val));
                          },
                          *parameters.sync,
                          [this] (float val)
                          {
                              rateSyncActive = val > 0.0f;

                              if (rateSyncActive)
                                  updateRateSync();
                              else
                                  rateSmoother.setTargetValue (parameters.rateFree->convertFrom0to1 (parameters.rateFree->getValue()));
                          },
                          *parameters.rateFree,
                          [this] (float val)
                          {
                              if (! rateSyncActive)
                                  rateSmoother.setTargetValue (val);
                          },
                          *parameters.rateSync,
                          [this] (float)
                          {
                              if (rateSyncActive)
                                  updateRateSync();
                          },
                          *parameters.amount,
                          [this] (float val)
                          {
                              amountSmoother.setTargetValue (val);
                          },
                          *parameters.blend,
                          [this] (float val)
                          {
                              blendSmoother.setTargetValue (val);
                          },
                          *parameters.offset,
                          [this] (float val)
                          {
                              offsetSmoother.setTargetValue (val);
                          },
                          *parameters.safeBass,
                          [this] (float val)
                          {
                              highpassSmoother.setTargetValue (val);
                          },
                          *parameters.warmth,
                          [this] (float val)
                          {
                              lowpassSmoother.setTargetValue (juce::mapToLog10 (1.0f - val, 1e3f, 22e3f));
                          },                          
                          *parameters.output,
                          [this] (float val)
                          {
                              outputSmoother.setTargetValue (val);
                          },
                          *parameters.mix,
                          [this] (float val)
                          {
                              mixSmoother.setTargetValue (val);
                          });

    LfoTable::initSpline<numSplinePoints>();
}

void Phaser::reset()
{
    auto apfSpec = getProcessSpec();
    apfSpec.numChannels = maxNotches;

    for (auto& apf : allpassFilters)
        apf.prepare (apfSpec);

    highpass.prepare (getNumChannels());
    highpass.calcCoefs (highpassSmoother.getCurrentValue(), 0.71f, getSampleRate());
    highpass.reset();

    lowpass.prepare (getNumChannels());
    lowpass.calcCoefs (lowpassSmoother.getCurrentValue(), getSampleRate());
    lowpass.reset();

    phasor.setFrequency (rateSmoother.getCurrentValue(), getSampleRate());
    phasor.setPhase (0.0);

    prevFilterOut.resize (getNumChannels());
    std::fill (prevFilterOut.begin(), prevFilterOut.end(), 0.0f);

    feedbackSmoother.reset (getSampleRate(), smoothingTimeSeconds);
    rateSmoother.reset (getSampleRate(), smoothingTimeSeconds);
    amountSmoother.reset (getSampleRate(), smoothingTimeSeconds);
    offsetSmoother.reset (getSampleRate(), smoothingTimeSeconds);
    highpassSmoother.reset (getSampleRate(), smoothingTimeSeconds);
    lowpassSmoother.reset (getSampleRate(), smoothingTimeSeconds);
    outputSmoother.reset (getSampleRate(), smoothingTimeSeconds);
    mixSmoother.reset (getSampleRate(), smoothingTimeSeconds);
}

void Phaser::process (float* const* buffer, int startIndex, int numSamples)
{
    const int endIndex = startIndex + numSamples;
    for (int i = startIndex; i < endIndex; i++)
    {
        if (rateSmoother.isSmoothing())
            phasor.setFrequency (rateSmoother.getNextValue(), getSampleRate());

        if (highpassSmoother.isSmoothing())
            highpass.calcCoefs (highpassSmoother.getNextValue(), 0.71f, getSampleRate());

        if (lowpassSmoother.isSmoothing())
            lowpass.calcCoefs (lowpassSmoother.getNextValue(), getSampleRate());

        const float feedback = feedbackSmoother.getNextValue();
        const float amount = amountSmoother.getNextValue();
        const float blend = blendSmoother.getNextValue();
        const float relativePhaseOffset = offsetSmoother.getNextValue();
        const float outputGain = juce::Decibels::decibelsToGain (outputSmoother.getNextValue());
        const float mix = mixSmoother.getNextValue();
        const float center = centerSmoother.getNextValue();
        const float spread = spreadSmoother.getNextValue();

        const auto [dryGain, wetGain] = AudioUtils::constantPowerMix (mix);

        for (int j = 0; j < getNumChannels(); j++)
        {
            const float offset = j == 0 ? 0.0f : relativePhaseOffset;
            const float lfo = LfoTable::getSpline<numSplinePoints> (shape, phasor.getFloat (offset));
            const float modVal = 2.0f * lfo - 1.0f;
            const float modCenter = juce::jlimit (0.0f, 1.0f, center + modAmplitude * amount * modVal * (1.0f - blend));
            const float modSpread = juce::jlimit (0.0f, 1.0f, spread + modAmplitude * amount * modVal * blend);
            const float cutoff = juce::mapToLog10 (modCenter, cutoffRangeStart, cutoffRangeEnd);
            const float resonance = juce::mapToLog10 (modSpread, resonanceRangeStart, resonanceRangeEnd);

            allpassFilters[j].setCutoffFrequency<false> (cutoff);
            allpassFilters[j].setQValue<true> (resonance);

            const float dry = buffer[j][i];
            const float apfIn = dry + feedback * feedbackSign * prevFilterOut[j];

            float apfOut = apfIn;

            for (int k = 0; k < notches; k++)
                apfOut = allpassFilters[j].processSample (k, apfOut);

            prevFilterOut[j] = apfOut;

            const float ff = highpass.processSample (apfIn, j);
            const float wet = 0.5f * (apfOut + feedbackSign * ff);
            const float lpfOut = lowpass.processSample (wet, j);
            const float y = dry * dryGain + lpfOut * wetGain * outputGain;

            buffer[j][i] = y;
        }

        phasor.inc();
    }
}

void Phaser::playbackStateChanged()
{
    if (getPlaybackState())
    {
        if (rateSyncActive)
            updateRateSync();
        else
            phasor.setPhase (0.0f);
    }
}

void Phaser::tempoChanged()
{
    if (rateSyncActive)
        updateRateSync();
}

void Phaser::updateRateSync()
{
    const double periodInBeats = ParamUtils::syncedRateIndexToBeats (parameters.rateSync->convertFrom0to1 (parameters.rateSync->getValue()));

    rateSmoother.setTargetValue (1.0f / (periodInBeats * getSecondsPerBeat()));
    phasor.setPhase (std::fmod (getPlaybackPosition(), periodInBeats) / periodInBeats);
}

} // namespace cgo
