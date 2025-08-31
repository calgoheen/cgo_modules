#include <cgo_processors/cgo_processors.h>

namespace cgo
{

Flanger::Flanger (Params& params)
    : parameters (params)
{
    addParameterUpdaters (*parameters.delay, [this] (float val)
                          {
                              delaySmoother.setTargetValue (val);
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
                              shape = static_cast<LfoTable::Shape> (val);
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
                          *parameters.depth,
                          [this] (float val)
                          {
                              depthSmoother.setTargetValue (val);
                          },
                          *parameters.offset,
                          [this] (float val)
                          {
                              offsetSmoother.setTargetValue (val);
                          },
                          *parameters.cutoff,
                          [this] (float val)
                          {
                              cutoffSmoother.setTargetValue (val);
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

void Flanger::reset()
{
    modulatedDelay.emplace (static_cast<int> (getSampleRate() * maxDelaySeconds) + 1);
    modulatedDelay->prepare (getProcessSpec());

    delaySmoother.reset (getSampleRate(), smoothingTimeSeconds);
    feedbackSmoother.reset (getSampleRate(), smoothingTimeSeconds);
    rateSmoother.reset (getSampleRate(), smoothingTimeSeconds);
    depthSmoother.reset (getSampleRate(), smoothingTimeSeconds);
    offsetSmoother.reset (getSampleRate(), smoothingTimeSeconds);
    cutoffSmoother.reset (getSampleRate(), smoothingTimeSeconds);
    outputSmoother.reset (getSampleRate(), smoothingTimeSeconds);
    mixSmoother.reset (getSampleRate(), smoothingTimeSeconds);

    highpass.prepare (getNumChannels());
    highpass.calcCoefs (cutoffSmoother.getCurrentValue(), 0.71f, getSampleRate());
    highpass.reset();
    phasor.setFrequency (rateSmoother.getCurrentValue(), getSampleRate());
    phasor.setPhase (0.0);
}

void Flanger::process (float* const* buffer, int startIndex, int numSamples)
{
    static constexpr float modCurve = -0.35f;
    static constexpr float maxDelayRatio = 64.0f;

    const int endIndex = startIndex + numSamples;
    for (int i = startIndex; i < endIndex; i++)
    {
        if (rateSmoother.isSmoothing())
            phasor.setFrequency (rateSmoother.getNextValue(), getSampleRate());

        if (cutoffSmoother.isSmoothing())
            highpass.calcCoefs (cutoffSmoother.getNextValue(), 0.71f, getSampleRate());

        const float maxDelaySamples = delaySmoother.getNextValue() * getSampleRate();
        const float feedback = feedbackSmoother.getNextValue() * feedbackSign;
        const float depth = depthSmoother.getNextValue();
        const float relativePhaseOffset = offsetSmoother.getNextValue();
        const float outputGain = outputSmoother.getNextValue();
        const float mix = mixSmoother.getNextValue();

        const auto [dryGain, wetGain] = AudioUtils::constantPowerMix (mix);

        const float minDelaySamples = maxDelaySamples / maxDelayRatio;

        for (int j = 0; j < getNumChannels(); j++)
        {
            const float offset = j == 0 ? 0.0f : relativePhaseOffset;
            const float modVal = LfoTable::getSpline<numSplinePoints> (shape, phasor.getFloat (offset));
            const float nextDelay = juce::jmap (Curve::exponential (depth * modVal, modCurve),
                                                maxDelaySamples,
                                                minDelaySamples < 2.0f ? 2.0f : minDelaySamples);

            const float dry = buffer[j][i];
            const float filterOut = highpass.processSample (dry, j);
            const float modulatedDelayOut = modulatedDelay->popSample (j, nextDelay, true);
            const float modulatedDelayIn = filterOut + feedback * modulatedDelayOut;
            modulatedDelay->pushSample (j, modulatedDelayIn);

            const float wet = 0.71f * (filterOut + modulatedDelayOut);
            const float y = dry * dryGain + wet * wetGain;

            buffer[j][i] = y;
        }

        phasor.inc();
    }
}

void Flanger::playbackStateChanged()
{
    if (getPlaybackState())
    {
        if (rateSyncActive)
            updateRateSync();
        else
            phasor.setPhase (0.0f);
    }
}

void Flanger::tempoChanged()
{
    if (rateSyncActive)
        updateRateSync();
}

void Flanger::updateRateSync()
{
    const double periodInBeats = ParamUtils::syncedRateIndexToBeats (parameters.rateSync->convertFrom0to1 (parameters.rateSync->getValue()));

    rateSmoother.setTargetValue (1.0f / (periodInBeats * getSecondsPerBeat()));
    phasor.setPhase (std::fmod (getPlaybackPosition(), periodInBeats) / periodInBeats);
}

} // namespace cgo
