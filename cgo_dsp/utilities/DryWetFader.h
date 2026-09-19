namespace cgo::dsp
{

class DryWetFader
{
public:
    DryWetFader() = default;

    void prepare (int numChannels, int maxBlockSize, int fadeLengthSamples, int latencySamples);
    void reset();
    void pushDry (const juce::AudioBuffer<float>& input);
    void mix (juce::AudioBuffer<float>& wet);

    void setTarget (bool wet);
    bool isDry() const;

private:
    float advance (float from, int numSamples) const;

    juce::AudioBuffer<float> history;
    int latency { 0 };
    int writePos { 0 };
    float increment { 1.0f };
    float gain { 1.0f };
    float target { 1.0f };

    JUCE_DECLARE_NON_COPYABLE (DryWetFader)
};

} // namespace cgo::dsp
