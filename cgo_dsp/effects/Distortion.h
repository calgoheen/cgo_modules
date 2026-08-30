namespace cgo::dsp
{

class Distortion
{
public:
    enum class Type
    {
        soft = 0,
        hard,
        tanh,
        sine,
        fold
    };

    static constexpr int latencySamples = 1;

    Distortion (int numChans)
      : numChannels (numChans),
        softClipper (&lutCache.get(), lutRange, lutPoints),
        hardClipper (&lutCache.get(), lutRange, lutPoints),
        tanhClipper (&lutCache.get(), lutRange, lutPoints),
        sineClipper (&lutCache.get(), lutRange, lutPoints),
        wavefolder (&lutCache.get(), lutRange, lutPoints)
    {
        jassert (numChannels == 1 || numChannels == 2);

        softClipper.prepare (numChannels);
        hardClipper.prepare (numChannels);
        tanhClipper.prepare (numChannels);
        sineClipper.prepare (numChannels);
        wavefolder.prepare (numChannels);

        reset();
    }

    ~Distortion() = default;

    void reset()
    {
        resetShapers();

        previous.fill (0.0f);
    }

    void process (float* const* buffer, int startSample, int numSamples)
    {
        for (int n = 0; n < numSamples; n++)
        {
            const int index = startSample + n;

            for (int j = 0; j < numChannels; j++)
            {
                const float x = buffer[j][index];

                const float dry = previous[(size_t) j];
                previous[(size_t) j] = x;

                const float wet = output * processShaper (drive * x, j);

                buffer[j][index] = dry + mix * (wet - dry);
            }
        }
    }

    void setType (Type t)
    {
        if (t == type)
            return;

        type = t;

        resetShapers();
    }

    void setDrive (float linearGain) { drive = linearGain; }

    void setOutputGain (float linearGain) { output = linearGain; }

    void setMix (float amount) { mix = juce::jlimit (0.0f, 1.0f, amount); }

private:
    static constexpr int maxChannels = 2;
    static constexpr float lutRange = 6.0f;
    static constexpr int lutPoints = 1 << 15;

    static constexpr float foldGain = 0.2f;

    float processShaper (float x, int channel)
    {
        x = juce::jlimit (-lutRange, lutRange, x);

        switch (type)
        {
            case Type::soft:
                return softClipper.processSample (x, channel);
            case Type::hard:
                return hardClipper.processSample (x, channel);
            case Type::tanh:
                return tanhClipper.processSample (x, channel);
            case Type::sine:
                return sineClipper.processSample (x, channel);
            case Type::fold:
                return foldGain * wavefolder.processSample (x, channel);
        }

        jassertfalse;
        return x;
    }

    void resetShapers()
    {
        softClipper.reset();
        hardClipper.reset();
        tanhClipper.reset();
        sineClipper.reset();
        wavefolder.reset();
    }

    const int numChannels;

    chowdsp::SharedLookupTableCache lutCache;

    chowdsp::ADAASoftClipper<float> softClipper;
    chowdsp::ADAAHardClipper<float> hardClipper;
    chowdsp::ADAATanhClipper<float> tanhClipper;
    chowdsp::ADAASineClipper<float> sineClipper;
    chowdsp::WestCoastWavefolder<float> wavefolder;

    Type type { Type::soft };

    float drive { 1.0f };
    float output { 1.0f };
    float mix { 1.0f };

    std::array<float, maxChannels> previous {};

    JUCE_DECLARE_NON_COPYABLE (Distortion)
};

} // namespace cgo::dsp
