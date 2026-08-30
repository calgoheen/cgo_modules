namespace cgo::dsp
{

class Chorus
{
public:
    enum class Algorithm
    {
        chorus = 0,
        ensemble
    };

    Chorus (float sr, int numChans)
      : sampleRate (sr),
        numChannels (numChans),
        modulatedDelays { DelayLine ((int) (sampleRate * delayLengthSeconds) + 1), DelayLine ((int) (sampleRate * delayLengthSeconds) + 1) }
    {
        jassert (numChannels == 1 || numChannels == 2);

        LfoShape::init();

        for (auto& d : modulatedDelays)
            d.prepare ({ 0.0, 0, (juce::uint32) numChannels });

        lowpass.prepare (numChannels);

        reset();
    }

    ~Chorus() = default;

    void reset()
    {
        for (auto& d : modulatedDelays)
            d.reset();

        lowpass.reset();
        phasor.setPhase (0.0);
    }

    void process (float* const* buffer, int startSample, int numSamples)
    {
        const auto config = algorithm == Algorithm::chorus ? chorusConfig() : ensembleConfig();

        for (int n = 0; n < numSamples; n++)
        {
            const int index = startSample + n;

            const StereoSample dry = { buffer[0][index], buffer[numChannels == 1 ? 0 : 1][index] };
            StereoSample wet = processTaps (dry, config);

            if (numChannels == 1)
            {
                buffer[0][index] = dryGain * dry[0] + wetGain * outputGain * lowpass.processSample (wet[0], 0);
            }
            else
            {
                AudioUtils::stereoWidth (wet[0], wet[1], width);

                buffer[0][index] = dryGain * dry[0] + wetGain * outputGain * lowpass.processSample (wet[0], 0);
                buffer[1][index] = dryGain * dry[1] + wetGain * outputGain * lowpass.processSample (wet[1], 1);
            }

            phasor.inc();
        }
    }

    void setAlgorithm (Algorithm a) { algorithm = a; }

    void setRate (float rateHz) { phasor.setFrequency (rateHz, sampleRate); }

    void setPhase (double phase) { phasor.setPhase (phase); }

    void setAmount (float amount)
    {
        static constexpr float amountCurve = 0.15f;

        shapedAmount = Curve::exponential (juce::jlimit (0.0f, 1.0f, amount), amountCurve);
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

    void setWidth (float amount) { width = amount; }

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
    using StereoSample = std::array<float, 2>;
    using DelayLine = chowdsp::DelayLine<float, chowdsp::DelayLineInterpolationTypes::Lagrange3rd>;
    using LowpassFilter = chowdsp::FirstOrderLPF<float>;

    static constexpr int numTaps = 2;
    static constexpr float delayLengthSeconds = 17e-3f;
    static constexpr float minDelaySamples = 2.0f;

    // Declared after the constants they are sized by, so they cannot join the aliases above.
    using TapDelays = std::array<float, numTaps>;

    struct TapConfig
    {
        TapDelays shortestDelay;
        float longestDelay;
        std::array<TapDelays, 2> phases;
    };

    TapConfig chorusConfig() const
    {
        static constexpr float maxDelaySeconds = 10e-3f;
        static constexpr auto phases = std::array { std::array { 0.0f, 0.75f }, std::array { 0.75f, 0.0f } };

        const float longestDelay = shapedAmount * maxDelaySeconds * sampleRate;

        return { TapDelays { minDelaySamples, juce::jmax (longestDelay / 2.0f, minDelaySamples) }, juce::jmax (longestDelay, minDelaySamples), phases };
    }

    TapConfig ensembleConfig() const
    {
        static constexpr float delayCenterSeconds = 13.5e-3f;
        static constexpr float delayRangeSeconds = 3.5e-3f;
        static constexpr auto phases = std::array { std::array { 0.0f, 1.0f / 3.0f }, std::array { 2.0f / 3.0f, 0.0f } };

        const float shortestDelay = (delayCenterSeconds - delayRangeSeconds * shapedAmount) * sampleRate;

        return { TapDelays { shortestDelay, shortestDelay }, (delayCenterSeconds + delayRangeSeconds * shapedAmount) * sampleRate, phases };
    }

    StereoSample processTaps (StereoSample dry, const TapConfig& config)
    {
        StereoSample wet = { 0.0f, 0.0f };

        for (int j = 0; j < numChannels; j++)
        {
            for (int k = 0; k < numTaps; k++)
            {
                const float modVal = LfoShape::get (LfoShape::sine, phasor.getFloat (config.phases[(size_t) j][(size_t) k]), (float) phasor.getFrequency());
                const float d = juce::jmap (modVal, config.shortestDelay[(size_t) k], config.longestDelay);

                const float delayOut = modulatedDelays[(size_t) k].popSample (j, d, true);
                modulatedDelays[(size_t) k].pushSample (j, dry[(size_t) j] + feedback * delayOut);
                wet[(size_t) j] += delayOut;
            }

            wet[(size_t) j] *= 0.5f;
        }

        return wet;
    }

    void updateFeedback()
    {
        static constexpr float maxFeedback = 0.99f;

        feedback = maxFeedback * feedbackAmount * feedbackSign;
    }

    const float sampleRate;
    const int numChannels;

    Algorithm algorithm { Algorithm::chorus };
    Phasor phasor;

    float shapedAmount { 0.0f };
    float feedbackAmount { 0.0f };
    float feedbackSign { 1.0f };
    float feedback { 0.0f };
    float width { 1.0f };
    float dryGain { 0.0f };
    float wetGain { 1.0f };
    float outputGain { 1.0f };

    std::array<DelayLine, numTaps> modulatedDelays;
    LowpassFilter lowpass;

    JUCE_DECLARE_NON_COPYABLE (Chorus)
};

} // namespace cgo::dsp
