namespace cgo
{

template <int N>
class DiffusionStep
{
public:
    using Array = std::array<float, N>;

    DiffusionStep (int maxDelaySamples, float basePhaseOffset)
        : delay (static_cast<int> (std::ceil (maxDelaySamples * (1.0f + maxModDepth))))
    {
        delay.prepare ({ 0.0, 0, N });

        for (int i = 0; i < N; i++)
        {
            polarity[i] = juce::Random::getSystemRandom().nextBool() ? -1.0f : 1.0f;
            spacing[i] = juce::Random::getSystemRandom().nextFloat();
            phaseOffsets[i] = std::fmod (static_cast<float> (i) / N + basePhaseOffset, 1.0f);
        }

        setDelay (0.0f);
    }

    ~DiffusionStep() = default;

    void reset()
    {
        delay.reset();
        phasor.setPhase (0.0);
    }

    Array process (Array x)
    {
        Array y;

        for (int i = 0; i < N; i++)
        {
            const float lfo = LfoTable::getLinear<numLfoPoints> (LfoTable::sine, phasor.getFloat (phaseOffsets[i]));
            const float modVal = 2.0f * lfo - 1.0f;
            const float modDelayLength = delayLengthSamples[i] * (1.0f + maxModDepth * modDepth * modVal);

            delay.pushSample (i, x[i]);
            y[i] = delay.popSample (i, modDelayLength, true);
        }

        ArrayMixers::Hadamard<float, N>::mix (y);

        for (int i = 0; i < N; i++)
            y[i] *= polarity[i];

        phasor.inc();

        return y;
    }

    void setDelay (float lengthInSamples)
    {
        const float scale = 1.0f / N;

        for (int i = 0; i < N; i++)
        {
            const float min = lengthInSamples * i * scale;
            const float max = lengthInSamples * (i + 1) * scale;
            delayLengthSamples[i] = juce::jmap (spacing[i], min, max);
        }
    }

    void setModRate (float rateHz, float sampleRate)
    {
        phasor.setFrequency (rateHz, sampleRate);
    }

    void setModDepth (float depth)
    {
        modDepth = depth;
    }

private:
    static constexpr int numLfoPoints = 256;
    static constexpr float maxModDepth = 0.05f;

    Array delayLengthSamples;
    Array polarity;
    Array spacing;
    Array phaseOffsets;

    Phasor phasor;
    float modDepth { 0.0f };

    using Delay = chowdsp::DelayLine<float, chowdsp::DelayLineInterpolationTypes::Linear>;
    Delay delay;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DiffusionStep)
};

} // namespace cgo
