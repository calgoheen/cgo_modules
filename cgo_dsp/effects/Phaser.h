namespace cgo::dsp
{

class Phaser
{
public:
    static constexpr int maxNotches = 42;

    Phaser (float sr, int numChans) : sampleRate (sr), numChannels (numChans)
    {
        jassert (numChannels <= maxNumChannels);

        LfoShape::init();

        for (auto& apf : allpassFilters)
            apf.prepare ({ (double) sampleRate, 0, (juce::uint32) maxNotches });

        highpass.prepare (numChannels);
        lowpass.prepare (numChannels);

        reset();
    }

    ~Phaser() = default;

    void reset()
    {
        for (auto& apf : allpassFilters)
            apf.reset();

        highpass.reset();
        lowpass.reset();
        phasor.setPhase (0.0);
        prevFilterOut.fill (0.0f);
    }

    void process (float* const* buffer, int startSample, int numSamples)
    {
        static constexpr float modAmplitude = 0.4f;

        for (int n = 0; n < numSamples; n++)
        {
            const int index = startSample + n;

            for (int j = 0; j < numChannels; j++)
            {
                const float x = buffer[j][index];

                auto& apf = allpassFilters[(size_t) j];
                float& prevOut = prevFilterOut[(size_t) j];

                const float offset = j == 0 ? 0.0f : stereoPhaseOffset;
                const float modVal = 2.0f * LfoShape::get (shape, phasor.getFloat (offset), (float) phasor.getFrequency()) - 1.0f;
                const float modCenter = juce::jlimit (0.0f, 1.0f, center + modAmplitude * modAmount * modVal * (1.0f - blend));
                const float modSpread = juce::jlimit (0.0f, 1.0f, spread + modAmplitude * modAmount * modVal * blend);

                apf.setCutoffFrequency<false> (juce::mapToLog10 (modCenter, cutoffRangeStart, cutoffRangeEnd));
                apf.setQValue<true> (juce::mapToLog10 (modSpread, resonanceRangeStart, resonanceRangeEnd));

                const float apfIn = x + feedback * prevOut;

                float apfOut = apfIn;

                for (int i = 0; i < notches; i++)
                    apfOut = apf.processSample (i, apfOut);

                prevOut = apfOut;

                const float hpfOut = highpass.processSample (apfIn, j);
                const float wet = 0.5f * (apfOut + feedbackSign * hpfOut);

                buffer[j][index] = x * dryGain + lowpass.processSample (wet, j) * wetGain * outputGain;
            }

            phasor.inc();
        }
    }

    void setRate (float rateHz) { phasor.setFrequency (rateHz, sampleRate); }

    void setPhase (double phase) { phasor.setPhase (phase); }

    void setShape (LfoShape::Shape s) { shape = s; }

    void setNotches (int numNotches) { notches = juce::jlimit (1, maxNotches, numNotches); }

    void setCenter (float frequencyHz) { center = juce::mapFromLog10 (frequencyHz, cutoffRangeStart, cutoffRangeEnd); }

    void setSpread (float amount)
    {
        static constexpr float resonanceParamRangeStart = 0.15f;
        static constexpr float resonanceParamRangeEnd = 2.25f;

        const float resonance = juce::mapToLog10 (1.0f - amount, resonanceParamRangeStart, resonanceParamRangeEnd);

        spread = juce::mapFromLog10 (resonance, resonanceRangeStart, resonanceRangeEnd);
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

    void setAmount (float amount) { modAmount = amount; }

    void setBlend (float amount) { blend = amount; }

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
    static constexpr int maxNumChannels = 2;
    static constexpr float cutoffRangeStart = 20.0f;
    static constexpr float cutoffRangeEnd = 22e3f;
    static constexpr float resonanceRangeStart = 3e-3f;
    static constexpr float resonanceRangeEnd = 15.0f;

    using HighpassFilter = chowdsp::SecondOrderHPF<float, chowdsp::CoefficientCalculators::CoefficientCalculationMode::Decramped>;
    using LowpassFilter = chowdsp::FirstOrderLPF<float>;
    using AllpassFilter = std::array<chowdsp::SVFAllpass<float>, maxNumChannels>;

    void updateFeedback()
    {
        static constexpr float maxFeedback = 0.99f;
        feedback = maxFeedback * feedbackAmount * feedbackSign;
    }

    const float sampleRate;
    const int numChannels;

    LfoShape::Shape shape { LfoShape::triangle };
    Phasor phasor;

    int notches { 1 };
    float center { 0.0f };
    float spread { 0.0f };
    float feedbackAmount { 0.0f };
    float feedbackSign { 1.0f };
    float feedback { 0.0f };
    float modAmount { 0.0f };
    float blend { 0.0f };
    float stereoPhaseOffset { 0.0f };
    float dryGain { 0.0f };
    float wetGain { 1.0f };
    float outputGain { 1.0f };

    std::array<float, maxNumChannels> prevFilterOut {};

    AllpassFilter allpassFilters;
    HighpassFilter highpass;
    LowpassFilter lowpass;

    JUCE_DECLARE_NON_COPYABLE (Phaser)
};

} // namespace cgo::dsp
