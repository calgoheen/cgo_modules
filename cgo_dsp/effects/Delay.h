namespace cgo::dsp
{

class Delay
{
public:
    enum class Mode
    {
        normal = 0,
        pingPong
    };

    static constexpr float maxDelaySeconds = 8.0f;

    Delay (float sr, int numChans) : sampleRate (sr), numChannels (numChans), delay ((int) (sampleRate * maxDelaySeconds) + 1)
    {
        jassert (numChannels == 1 || numChannels == 2);

        delay.prepare ({ 0.0, 0, (juce::uint32) numChannels });

        lowpass.prepare (numChannels);
        highpass.prepare (numChannels);

        setGlideLength (defaultGlideSeconds);
        updateFilters();

        reset();
    }

    ~Delay() = default;

    void reset()
    {
        delay.reset();
        lowpass.reset();
        highpass.reset();

        for (size_t i = 0; i < smoothers.size(); i++)
            smoothers[i].reset (targetDelaySamples[i]);
    }

    void process (float* const* buffer, int startSample, int numSamples)
    {
        const bool crossFeed = mode == Mode::pingPong && numChannels == 2;

        for (int n = 0; n < numSamples; n++)
        {
            const int index = startSample + n;

            std::array<float, 2> wet {};

            for (int j = 0; j < numChannels; j++)
            {
                const float d = smoothers[(size_t) j].process (targetDelaySamples[(size_t) j]);
                const float delayOut = delay.popSample (j, d, true);

                wet[(size_t) j] = lowpass.processSample (highpass.processSample (delayOut, j), j);
            }

            if (crossFeed)
            {
                const float in = 0.5f * (buffer[0][index] + buffer[1][index]);

                delay.pushSample (0, in + feedback * wet[1]);
                delay.pushSample (1, feedback * wet[0]);
            }
            else
            {
                for (int j = 0; j < numChannels; j++)
                    delay.pushSample (j, buffer[j][index] + feedback * wet[(size_t) j]);
            }

            for (int j = 0; j < numChannels; j++)
                buffer[j][index] = dryGain * buffer[j][index] + wetGain * outputGain * wet[(size_t) j];
        }
    }

    void setMode (Mode m) { mode = m; }

    void setDelayLeft (float lengthInSeconds) { setDelayTime (0, lengthInSeconds); }

    void setDelayRight (float lengthInSeconds) { setDelayTime (1, lengthInSeconds); }

    void setGlideLength (float lengthInSeconds)
    {
        const float lengthInSamples = juce::jmax (minGlideSamples, lengthInSeconds * sampleRate);

        for (auto& s : smoothers)
            s.setLength (lengthInSamples);
    }

    void setFeedback (float amount)
    {
        static constexpr float maxFeedback = 0.99f;

        feedback = maxFeedback * juce::jlimit (-1.0f, 1.0f, amount);
    }

    void setFilter (float centerFrequencyHz, float widthInOctaves)
    {
        centerFrequency = juce::jlimit (minCutoffHz, maxCutoffRatio * sampleRate, centerFrequencyHz);
        bandwidth = juce::jmax (minBandwidthOctaves, widthInOctaves);

        updateFilters();
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
    using LowpassFilter = chowdsp::SecondOrderLPF<float, chowdsp::CoefficientCalculators::CoefficientCalculationMode::Decramped>;
    using HighpassFilter = chowdsp::SecondOrderHPF<float, chowdsp::CoefficientCalculators::CoefficientCalculationMode::Decramped>;

    static constexpr float minDelaySamples = 2.0f;
    static constexpr float minGlideSamples = 1.0f;
    static constexpr float defaultGlideSeconds = 0.1f;
    static constexpr float minCutoffHz = 10.0f;
    static constexpr float maxCutoffRatio = 0.49f;
    static constexpr float minBandwidthOctaves = 0.5f;

    void setDelayTime (int side, float lengthInSeconds)
    {
        const float maxSamples = maxDelaySeconds * sampleRate;

        targetDelaySamples[(size_t) side] = juce::jlimit (minDelaySamples, maxSamples, lengthInSeconds * sampleRate);
    }

    void updateFilters()
    {
        static constexpr float filterQ = 0.7071f;

        const float halfSpan = std::pow (2.0f, 0.5f * bandwidth);

        lowpass.calcCoefs (juce::jmin (centerFrequency * halfSpan, maxCutoffRatio * sampleRate), filterQ, sampleRate);
        highpass.calcCoefs (juce::jmax (centerFrequency / halfSpan, minCutoffHz), filterQ, sampleRate);
    }

    const float sampleRate;
    const int numChannels;

    Mode mode { Mode::normal };

    std::array<float, 2> targetDelaySamples { minDelaySamples, minDelaySamples };
    float feedback { 0.0f };
    float centerFrequency { 1000.0f };
    float bandwidth { 10.0f };
    float dryGain { 0.0f };
    float wetGain { 1.0f };
    float outputGain { 1.0f };

    std::array<SmoothingFilter<float>, 2> smoothers;

    DelayLine delay;
    LowpassFilter lowpass;
    HighpassFilter highpass;

    JUCE_DECLARE_NON_COPYABLE (Delay)
};

} // namespace cgo::dsp
