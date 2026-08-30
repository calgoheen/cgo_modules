namespace cgo::dsp
{

class Crush
{
public:
    Crush (int numChans) : numChannels (numChans)
    {
        jassert (numChannels == 1 || numChannels == 2);

        reset();
    }

    ~Crush() = default;

    void reset()
    {
        held.fill (0.0f);
        counter = 0.0f;
    }

    void process (float* const* buffer, int startSample, int numSamples)
    {
        for (int n = 0; n < numSamples; n++)
        {
            const int index = startSample + n;

            if (counter <= 0.0f)
            {
                for (int j = 0; j < numChannels; j++)
                    held[(size_t) j] = quantize (buffer[j][index]);

                counter += downsampleFactor;
            }

            counter -= 1.0f;

            for (int j = 0; j < numChannels; j++)
                buffer[j][index] = held[(size_t) j];
        }
    }

    void setDownsampleFactor (float factor) { downsampleFactor = juce::jmax (1.0f, factor); }

    void setBitDepth (float bits)
    {
        levels = std::exp2 (juce::jlimit (minBitDepth, maxBitDepth, bits) - 1.0f);
        invLevels = 1.0f / levels;
    }

private:
    static constexpr int maxChannels = 2;
    static constexpr float minBitDepth = 1.0f;
    static constexpr float maxBitDepth = 16.0f;

    float quantize (float x) const { return std::round (x * levels) * invLevels; }

    const int numChannels;

    float downsampleFactor { 1.0f };
    float counter { 0.0f };
    float levels { 32768.0f };
    float invLevels { 1.0f / 32768.0f };

    std::array<float, maxChannels> held {};

    JUCE_DECLARE_NON_COPYABLE (Crush)
};

} // namespace cgo::dsp
