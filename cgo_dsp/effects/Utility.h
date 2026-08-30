namespace cgo::dsp
{

class Utility
{
public:
    Utility (int numChans) : numChannels (numChans) { jassert (numChannels == 1 || numChannels == 2); }

    ~Utility() = default;

    void process (float* const* buffer, int startSample, int numSamples)
    {
        for (int n = 0; n < numSamples; n++)
        {
            const int index = startSample + n;

            if (numChannels == 1)
            {
                buffer[0][index] *= polarity * gain;
                continue;
            }

            float l = polarity * buffer[0][index];
            float r = polarity * buffer[1][index];

            AudioUtils::stereoWidth (l, r, mono ? 0.0f : width);

            buffer[0][index] = l * leftGain * gain;
            buffer[1][index] = r * rightGain * gain;
        }
    }

    void setGain (float linearGain) { gain = linearGain; }

    void setPan (float amount)
    {
        const float pan = juce::jlimit (-1.0f, 1.0f, amount);

        leftGain = juce::jmin (1.0f, 1.0f - pan);
        rightGain = juce::jmin (1.0f, 1.0f + pan);
    }

    void setWidth (float amount) { width = amount; }

    void setMonoEnabled (bool shouldBeMono) { mono = shouldBeMono; }

    void setPolarityInverted (bool inverted) { polarity = inverted ? -1.0f : 1.0f; }

private:
    const int numChannels;

    float gain { 1.0f };
    float leftGain { 1.0f };
    float rightGain { 1.0f };
    float width { 1.0f };
    float polarity { 1.0f };
    bool mono { false };

    JUCE_DECLARE_NON_COPYABLE (Utility)
};

} // namespace cgo::dsp
