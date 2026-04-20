namespace cgo
{

enum class DiffusionReverbFilterType
{
    lowpass = 0,
    highpass
};

template <int N, int S>
class DiffusionReverb
{
public:
    using Array = std::array<float, N>;

    DiffusionReverb (float maxSizeSeconds, float sr)
        : sampleRate (sr),
        maxRoomSizeSamples (static_cast<int> (std::ceil (maxSizeSeconds * sampleRate)) + 1),
        maxPredelaySamples (static_cast<int> (std::ceil (maxPredelaySeconds * sampleRate)) + 1),
        fdn (maxRoomSizeSamples),
        earlyReflections (maxRoomSizeSamples / 2 + 1),
        diffuser (maxRoomSizeSamples),
        predelay (maxPredelaySamples)
    {
        predelay.prepare ({ 0.0, 0, 2 });
        predelay.reset();

        inputFilter.first.prepare (2);
        inputFilter.first.reset();
        inputFilter.first.calcCoefs (22e3f, 0.71f, sampleRate);

        inputFilter.second.prepare (2);
        inputFilter.second.reset();
        inputFilter.second.calcCoefs (10.0f, 0.71f, sampleRate);

        setSize (0.1f);
        setDecay (3.0f);
        setDecayFilter (MultiTapDelay::FilterType::lowShelf, 0.0f, 250.0f);
        setDecayFilter (MultiTapDelay::FilterType::highShelf, 0.0f, 2500.0f);
        setEarlyReflectionGain (1.0f);
        setDiffusionGain (1.0f);
        setPredelay (0.0005f);
    }

    ~DiffusionReverb() = default;

    void process (float& l, float& r)
    {
        predelay.pushSample (0, l);
        predelay.pushSample (1, r);
        l = predelay.popSample (0, predelaySamples, true);
        r = predelay.popSample (1, predelaySamples, true);

        l = inputFilter.first.processSample (l, 0);
        l = inputFilter.second.processSample (l, 0);
        r = inputFilter.first.processSample (r, 1);
        r = inputFilter.second.processSample (r, 1);

        Array y;
        for (int i = 0; i < N; i += 2)
        {
            y[i] = l;
            y[i + 1] = r;
        }

        const auto diffuserOut = diffuser.process (y);
        const auto erOut = earlyReflections.process (diffuserOut);
        y = fdn.process (diffuserOut);

        for (int i = 0; i < N; i++)
            y[i] = y[i] * diffusionGain + erOut[i] * earlyReflectionGain;

        l = 0.0f;
        r = 0.0f;
        for (int i = 0; i < N; i += 2)
        {
            l += y[i];
            r += y[i + 1];
        }

        l /= static_cast<float> (N / 2);
        r /= static_cast<float> (N / 2);
    }

    void reset()
    {
        fdn.reset();
        diffuser.reset();
        predelay.reset();
    }
    
    void setSize (float lengthInSeconds)
    {
        jassert (std::ceil (lengthInSeconds * sampleRate) < maxRoomSizeSamples);

        const float roomSizeSamples = lengthInSeconds * sampleRate;
        fdn.setDelay (roomSizeSamples);
        earlyReflections.setDelay (0.5f * roomSizeSamples);
        diffuser.setDelay (0.5f * roomSizeSamples);

        roomSizeSeconds = lengthInSeconds;
        updateDecayLength();
    }

    void setDecay (float lengthInSeconds)
    {
        decayLengthSeconds = lengthInSeconds;
        updateDecayLength();
    }

    void setInputFilter (DiffusionReverbFilterType filterType, float cutoff)
    {
        if (filterType == DiffusionReverbFilterType::lowpass)
            inputFilter.first.calcCoefs (cutoff, 0.71f, sampleRate);
        else
            inputFilter.second.calcCoefs (cutoff, 0.71f, sampleRate);
    }

    void setDecayFilter (MultiTapDelay::FilterType filterType, float gainInDb, float cutoffHz)
    {
        fdn.setFilterCoefs (filterType, gainInDb, cutoffHz, sampleRate);
    }

    void setEarlyReflectionGain (float linearGain)
    {
        earlyReflectionGain = linearGain;
    }

    void setDiffusionGain (float linearGain)
    {
        diffusionGain = linearGain;
    }

    void setTailModRate (float rateHz)
    {
        fdn.setModRate (rateHz, sampleRate);
    }

    void setTailModDepth (float depth)
    {
        fdn.setModDepth (depth);
    }

    void setDiffusionModRate (float rateHz)
    {
        diffuser.setModRate (rateHz, sampleRate);
    }

    void setDiffusionModDepth (float depth)
    {
        diffuser.setModDepth (depth);
    }

    void setPredelay (float lengthInSeconds)
    {
        predelaySamples = lengthInSeconds * sampleRate;
    }

private:
    void updateDecayLength()
    {
        const float numLoops = decayLengthSeconds / roomSizeSeconds;
        const float gainPerLoop = -60.0f / numLoops;
    
        fdn.setFeedback (std::pow (10.0f, gainPerLoop * 0.05f));
    }

    static constexpr float maxPredelaySeconds = 0.25f;

    const float sampleRate;
    const int maxRoomSizeSamples;
    const int maxPredelaySamples;

    float roomSizeSeconds;
    float decayLengthSeconds;
    float earlyReflectionGain;
    float diffusionGain;
    float predelaySamples;

    MultiTapDelay::Tail<N> fdn;
    MultiTapDelay::EarlyReflections<N, MultiTapDelay::SpacingType::linear> earlyReflections;
    Diffuser<N, S> diffuser;

    using PredelayLine = chowdsp::DelayLine<float, chowdsp::DelayLineInterpolationTypes::Linear>;
    PredelayLine predelay;

    using Filter = std::pair<chowdsp::SecondOrderLPF<float>, chowdsp::SecondOrderHPF<float>>;
    Filter inputFilter;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DiffusionReverb)
};

} // namespace cgo
