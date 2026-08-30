namespace cgo::dsp
{

class Compressor
{
public:
    using Mode = juce::dsp::BallisticsFilterLevelCalculationType;

    Compressor (float sr, int numChans) : sampleRate (sr), numChannels (numChans)
    {
        jassert (numChannels == 1 || numChannels == 2);

        // The channels are summed before detection, so one detector channel is enough
        detector.prepare ({ (double) sampleRate, 0, 1 });

        setAttack (10.0f);
        setRelease (100.0f);

        reset();
    }

    ~Compressor() = default;

    void reset()
    {
        detector.reset();
        gainDb = 0.0f;
    }

    void process (float* const* buffer, int startSample, int numSamples)
    {
        const float scale = 1.0f / (float) numChannels;

        for (int n = 0; n < numSamples; n++)
        {
            const int index = startSample + n;

            float sum = 0.0f;

            for (int j = 0; j < numChannels; j++)
                sum += buffer[j][index];

            const float level = detector.processSample (0, sum * scale);
            const float overDb = juce::Decibels::gainToDecibels (level, silenceDb) - thresholdDb;

            gainDb = computeGainDb (overDb);

            const float wetGain = juce::Decibels::decibelsToGain (gainDb + makeupDb);
            const float g = 1.0f - mix + mix * wetGain;

            for (int j = 0; j < numChannels; j++)
                buffer[j][index] *= g;
        }
    }

    void setThreshold (float thresholdInDb) { thresholdDb = thresholdInDb; }

    void setRatio (float amount) { slope = 1.0f / juce::jmax (1.0f, amount) - 1.0f; }

    void setKnee (float kneeInDb) { kneeDb = juce::jmax (0.0f, kneeInDb); }

    void setMakeup (float makeupInDb) { makeupDb = makeupInDb; }

    void setMix (float amount) { mix = juce::jlimit (0.0f, 1.0f, amount); }

    void setAttack (float attackInMs) { detector.setAttackTime (attackInMs); }

    void setRelease (float releaseInMs) { detector.setReleaseTime (releaseInMs); }

    void setMode (Mode m)
    {
        if (m == mode)
            return;

        mode = m;
        detector.setLevelCalculationType (m);
    }

private:
    static constexpr float silenceDb = -140.0f;

    float computeGainDb (float overDb) const
    {
        const float halfKnee = 0.5f * kneeDb;

        if (overDb >= halfKnee)
            return slope * overDb;

        if (overDb <= -halfKnee)
            return 0.0f;

        const float x = overDb + halfKnee;
        return slope * x * x / (2.0f * kneeDb);
    }

    const float sampleRate;
    const int numChannels;

    float thresholdDb { 0.0f };
    float slope { 0.0f };
    float kneeDb { 6.0f };
    float makeupDb { 0.0f };
    float mix { 1.0f };
    float gainDb { 0.0f };
    Mode mode { Mode::peak };

    juce::dsp::BallisticsFilter<float> detector;

    JUCE_DECLARE_NON_COPYABLE (Compressor)
};

} // namespace cgo::dsp
