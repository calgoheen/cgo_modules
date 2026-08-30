namespace cgo::dsp
{

class Filter
{
public:
    enum class Type
    {
        lowpass = 0,
        bandpass,
        highpass,
        notch
    };

    enum class Order
    {
        twoPole = 0,
        fourPole
    };

    Filter (float sr, int numChans) : sampleRate (sr), numChannels (numChans)
    {
        for (auto& s : stages)
            s.prepare ({ (double) sampleRate, 0, (juce::uint32) numChannels });

        updateCoefficients();

        reset();
    }

    ~Filter() = default;

    void reset()
    {
        for (auto& s : stages)
            s.reset();
    }

    void process (float* const* buffer, int startSample, int numSamples)
    {
        const int activeStages = getNumActiveStages();

        for (int n = 0; n < numSamples; n++)
        {
            const int index = startSample + n;

            for (int j = 0; j < numChannels; j++)
            {
                float x = buffer[j][index];

                for (int s = 0; s < activeStages; s++)
                    x = processStage (stages[(size_t) s], j, x);

                buffer[j][index] = x;
            }
        }
    }

    void setType (Type t) { type = t; }

    void setOrder (Order o)
    {
        if (o == order)
            return;

        order = o;

        if (order == Order::fourPole)
            stages[1].reset();

        updateCoefficients();
    }

    void setFilter (float cutoffHz, float resonance)
    {
        cutoff = juce::jlimit (minCutoffHz, maxCutoffRatio * sampleRate, cutoffHz);
        q = juce::jmax (minResonance, resonance);

        updateCoefficients();
    }

private:
    using Stage = chowdsp::StateVariableFilter<float, chowdsp::StateVariableFilterType::MultiMode>;

    static constexpr float minCutoffHz = 10.0f;
    static constexpr float maxCutoffRatio = 0.49f;
    static constexpr float minResonance = 0.1f;

    int getNumActiveStages() const { return order == Order::fourPole ? 2 : 1; }

    float processStage (Stage& stage, int channel, float x)
    {
        const auto [high, band, low] = stage.processCore (x, stage.ic1eq[(size_t) channel], stage.ic2eq[(size_t) channel]);

        switch (type)
        {
            case Type::lowpass:
                return low;
            case Type::bandpass:
                return bandpassGain * band;
            case Type::highpass:
                return high;
            case Type::notch:
                return low + high;
        }

        jassertfalse;
        return x;
    }

    void updateCoefficients()
    {
        const float stageQ = order == Order::fourPole ? std::sqrt (q) : q;

        bandpassGain = 1.0f / stageQ;

        const int activeStages = getNumActiveStages();

        for (int s = 0; s < activeStages; s++)
        {
            stages[(size_t) s].setCutoffFrequency<false> (cutoff);
            stages[(size_t) s].setQValue<true> (stageQ);
        }
    }

    const float sampleRate;
    const int numChannels;

    Type type { Type::lowpass };
    Order order { Order::twoPole };

    float cutoff { 1000.0f };
    float q { 0.7071f };
    float bandpassGain { 1.0f };

    std::array<Stage, 2> stages;

    JUCE_DECLARE_NON_COPYABLE (Filter)
};

} // namespace cgo::dsp
