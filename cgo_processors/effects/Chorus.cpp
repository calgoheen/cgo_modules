#include <cgo_processors/cgo_processors.h>

namespace cgo
{

Chorus::Chorus (Params& params)
    : parameters (params)
{
    addParameterUpdaters (*parameters.type,
                          [this] (float val)
                          {
                              algorithm = static_cast<Algorithm> (val);
                          },
                          *parameters.rate,
                          [this] (float val)
                          {
                              rateSmoother.setTargetValue (val);
                          },
                          *parameters.amount,
                          [this] (float val)
                          {
                              amountSmoother.setTargetValue (val);
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
                          *parameters.width,
                          [this] (float val)
                          {
                              widthSmoother.setTargetValue (val);
                          },
                          *parameters.warmth,
                          [this] (float val)
                          {
                              const float cutoff = juce::mapToLog10 (val, 22e3f, 1000.0f);
                              cutoffSmoother.setTargetValue (cutoff);
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

void Chorus::reset()
{
    for (auto& d : modulatedDelays)
    {
        d.emplace (static_cast<int> (getSampleRate() * delayLengthSeconds) + 1);
        d->prepare (getProcessSpec());
    }

    rateSmoother.reset (getSampleRate(), smoothingTimeSeconds);
    amountSmoother.reset (getSampleRate(), smoothingTimeSeconds);
    feedbackSmoother.reset (getSampleRate(), smoothingTimeSeconds);
    widthSmoother.reset (getSampleRate(), smoothingTimeSeconds);
    cutoffSmoother.reset (getSampleRate(), smoothingTimeSeconds);
    outputSmoother.reset (getSampleRate(), smoothingTimeSeconds);
    mixSmoother.reset (getSampleRate(), smoothingTimeSeconds);

    lowpass.prepare (getNumChannels());
    lowpass.calcCoefs (cutoffSmoother.getCurrentValue(), getSampleRate());
    lowpass.reset();

    phasor.setFrequency (rateSmoother.getCurrentValue(), getSampleRate());
    phasor.setPhase (0.0);
}

void Chorus::process (float* const* buffer, int startIndex, int numSamples)
{
    static constexpr float amountCurve = 0.15f;

    const int endIndex = startIndex + numSamples;
    for (int i = startIndex; i < endIndex; i++)
    {
        if (rateSmoother.isSmoothing())
            phasor.setFrequency (rateSmoother.getNextValue(), getSampleRate());

        if (cutoffSmoother.isSmoothing())
            lowpass.calcCoefs (cutoffSmoother.getNextValue(), getSampleRate());

        const float amount = Curve::exponential (juce::jlimit (0.0f, 1.0f, amountSmoother.getNextValue()), amountCurve);
        const float feedback = feedbackSmoother.getNextValue() * feedbackSign;
        const float width = widthSmoother.getNextValue();
        const float outputGain = juce::Decibels::decibelsToGain (outputSmoother.getNextValue());
        const float mix = mixSmoother.getNextValue();

        const StereoSample dry = { buffer[0][i], buffer[1][i] };
        StereoSample wet = algorithm == Algorithm::chorus ? processChorus (dry, amount, feedback)
                                                          : processEnsemble (dry, amount, feedback);
        
        AudioUtils::stereoWidth (wet[0], wet[1], width);
        const auto [dryGain, wetGain] = AudioUtils::constantPowerMix (mix);

        for (int j = 0; j < 2; j++)
        {
            const float lpfOut = lowpass.processSample (wet[j], j);
            buffer[j][i] = dryGain * dry[j] + wetGain * outputGain * lpfOut;
        }

        phasor.inc();
    }
}

void Chorus::playbackStateChanged()
{
    if (getPlaybackState())
        phasor.setPhase (0.0);
}

Chorus::StereoSample Chorus::processChorus (StereoSample dry, float amount, float feedback)
{
    static constexpr float maxDelaySeconds = 10e-3f;
    static constexpr int numTaps = 2;
    static constexpr auto phases = std::array { std::array { 0.0f, 0.75f }, std::array { 0.75f, 0.0f } };

    const float maxDelaySamples = amount * maxDelaySeconds * getSampleRate();

    StereoSample wet = { 0.0f, 0.0f };

    for (int j = 0; j < 2; j++)
    {
        for (int k = 0; k < numTaps; k++)
        {
            const float minDelaySamples = k == 0 ? 0.0f : maxDelaySamples / 2.0f;
            const float modVal = LfoTable::getSpline<numSplinePoints> (LfoTable::sine, phasor.getFloat (phases[j][k]));
            const float d = juce::jmap (modVal, 
                                        minDelaySamples < 2.0f ? 2.0f : minDelaySamples, 
                                        maxDelaySamples < 2.0f ? 2.0f : maxDelaySamples);
            
            const float delayOut = modulatedDelays[k]->popSample (j, d, true);
            modulatedDelays[k]->pushSample (j, dry[j] + feedback * delayOut);
            wet[j] += delayOut;
        }

        wet[j] *= 0.5f;
    }

    return wet;
}

Chorus::StereoSample Chorus::processEnsemble (StereoSample dry, float amount, float feedback)
{
    static constexpr float delayCenterSeconds = 13.5e-3f;
    static constexpr float delayRangeSeconds = 3.5e-3f;
    static constexpr int numTaps = 2;
    static constexpr auto phases = std::array { std::array { 0.0f, 1.0f / 3.0f }, std::array { 2.0f / 3.0f, 0.0f } };

    const float minDelaySamples = (delayCenterSeconds - delayRangeSeconds * amount) * getSampleRate();
    const float maxDelaySamples = (delayCenterSeconds + delayRangeSeconds * amount) * getSampleRate();

    StereoSample wet = { 0.0f, 0.0f };

    for (int j = 0; j < 2; j++)
    {
        for (int k = 0; k < numTaps; k++)
        {
            const float modVal = LfoTable::getSpline<numSplinePoints> (LfoTable::sine, phasor.getFloat (phases[j][k]));
            const float d = juce::jmap (modVal, minDelaySamples, maxDelaySamples);
            
            const float delayOut = modulatedDelays[k]->popSample (j, d, true);
            modulatedDelays[k]->pushSample (j, dry[j] + feedback * delayOut);
            wet[j] += delayOut;
        }

        wet[j] *= 0.5f;
    }

    return wet;
}

} // namespace cgo
