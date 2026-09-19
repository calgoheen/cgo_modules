namespace cgo::dsp
{

class Pitch
{
public:
    Pitch (int numChans) : numChannels (numChans)
    {
        jassert (numChannels == 1 || numChannels == 2);

        shifter.prepare ({ 0.0, 0, (juce::uint32) numChannels });
        shifter.setShiftFactor (1.0f);

        reset();
    }

    ~Pitch() = default;

    void reset() { shifter.reset(); }

    void process (float* const* buffer, int startSample, int numSamples)
    {
        for (int n = 0; n < numSamples; n++)
        {
            const int index = startSample + n;

            for (int j = 0; j < numChannels; j++)
            {
                const float dry = buffer[j][index];
                const float wet = shifter.processSample ((size_t) j, dry);

                buffer[j][index] = dryGain * dry + wetGain * wet;
            }
        }
    }

    void setShiftSemitones (float semitones) { shifter.setShiftFactor (std::pow (2.0f, semitones / 12.0f)); }

    void setMix (float amount)
    {
        const auto [dry, wet] = AudioUtils::constantPowerMix (amount);

        dryGain = dry;
        wetGain = wet;
    }

private:
    using Shifter = chowdsp::PitchShifter<float, chowdsp::DelayLineInterpolationTypes::Lagrange3rd>;

    static constexpr int bufferSize = 4096;
    static constexpr int crossfadeOverlap = 256;

    const int numChannels;

    Shifter shifter { bufferSize, crossfadeOverlap };

    float dryGain { 0.0f };
    float wetGain { 1.0f };

    JUCE_DECLARE_NON_COPYABLE (Pitch)
};

} // namespace cgo::dsp
