namespace cgo::dsp
{

class Flanger
{
public:
    static constexpr float minDelaySeconds = 0.1e-3f;
    static constexpr float maxDelaySeconds = 20e-3f;

    Flanger (float sr, int numChans) : sampleRate (sr), numChannels (numChans), modulatedDelay ((int) (sampleRate * maxDelaySeconds) + 1)
    {
        LfoShape::init();

        modulatedDelay.prepare ({ 0.0, 0, (juce::uint32) numChannels });

        highpass.prepare (numChannels);
        lowpass.prepare (numChannels);

        reset();
    }

    ~Flanger() = default;

    void reset()
    {
        modulatedDelay.reset();
        highpass.reset();
        lowpass.reset();
        phasor.setPhase (0.0);
    }

    void process (float* const* buffer, int startSample, int numSamples)
    {
        for (int n = 0; n < numSamples; n++)
        {
            const int index = startSample + n;

            for (int j = 0; j < numChannels; j++)
            {
                const float x = buffer[j][index];

                const float offset = j == 0 ? 0.0f : stereoPhaseOffset;
                const float modVal = LfoShape::get (shape, phasor.getFloat (offset), (float) phasor.getFrequency());
                const float nextDelay = juce::jmap (Curve::exponential (depth * modVal, modCurve), maxDelaySamples, shortestDelaySamples);

                const float delayOut = modulatedDelay.popSample (j, nextDelay, true);
                modulatedDelay.pushSample (j, x + feedback * highpass.processSample (delayOut, j));

                const float wet = 0.5f * lowpass.processSample (x + feedbackSign * delayOut, j);

                buffer[j][index] = x * dryGain + wet * wetGain * outputGain;
            }

            phasor.inc();
        }
    }

    void setRate (float rateHz) { phasor.setFrequency (rateHz, sampleRate); }

    void setPhase (double phase) { phasor.setPhase (phase); }

    void setShape (LfoShape::Shape s) { shape = s; }

    void setDelay (float lengthInSeconds)
    {
        maxDelaySamples = lengthInSeconds * sampleRate;
        shortestDelaySamples = juce::jmax (maxDelaySamples / maxDelayRatio, minDelaySamples);
    }

    void setFeedback (float amount)
    {
        feedbackAmount = amount;
        updateFeedback();
    }

    void setFeedbackFlipped (bool flipped)
    {
        feedbackSign = flipped ? -1.0f : 1.0f;
        updateFeedback();
    }

    void setDepth (float amount) { depth = amount; }

    void setStereoPhaseOffset (float phaseOffset) { stereoPhaseOffset = phaseOffset; }

    void setSafeBass (float cutoffHz)
    {
        static constexpr float safeBassQ = 0.71f;

        highpass.calcCoefs (cutoffHz, safeBassQ, sampleRate);
    }

    void setWarmth (float amount)
    {
        static constexpr float warmthMinCutoff = 22e3f;
        static constexpr float warmthMaxCutoff = 1000.0f;

        lowpass.calcCoefs (juce::mapToLog10 (amount, warmthMinCutoff, warmthMaxCutoff), sampleRate);
    }

    void setMix (float amount)
    {
        const auto [dry, wet] = AudioUtils::constantPowerMix (amount);

        dryGain = dry;
        wetGain = wet;
    }

    void setOutputGain (float linearGain) { outputGain = linearGain; }

private:
    using DelayLine = chowdsp::DelayLine<float, chowdsp::DelayLineInterpolationTypes::Lagrange3rd>;
    using HighpassFilter = chowdsp::SecondOrderHPF<float, chowdsp::CoefficientCalculators::CoefficientCalculationMode::Decramped>;
    using LowpassFilter = chowdsp::FirstOrderLPF<float>;

    static constexpr float minDelaySamples = 2.0f;
    static constexpr float maxDelayRatio = 32.0f;
    static inline const float modCurve = -std::log10 (maxDelayRatio) / 6.0f;

    void updateFeedback()
    {
        static constexpr float maxFeedback = 0.99f;

        feedback = maxFeedback * feedbackAmount * feedbackSign;
    }

    const float sampleRate;
    const int numChannels;

    LfoShape::Shape shape { LfoShape::triangle };
    Phasor phasor;

    float maxDelaySamples { 0.0f };
    float shortestDelaySamples { minDelaySamples };
    float feedbackAmount { 0.0f };
    float feedbackSign { 1.0f };
    float feedback { 0.0f };
    float depth { 1.0f };
    float stereoPhaseOffset { 0.0f };
    float dryGain { 0.0f };
    float wetGain { 1.0f };
    float outputGain { 1.0f };

    DelayLine modulatedDelay;
    HighpassFilter highpass;
    LowpassFilter lowpass;

    JUCE_DECLARE_NON_COPYABLE (Flanger)
};

} // namespace cgo::dsp
