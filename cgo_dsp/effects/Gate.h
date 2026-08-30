namespace cgo::dsp
{

class Gate
{
public:
    using Mode = juce::dsp::BallisticsFilterLevelCalculationType;

    Gate (float sr, int numChans) : sampleRate (sr), numChannels (numChans)
    {
        jassert (numChannels == 1 || numChannels == 2);

        detector.prepare ({ (double) sampleRate, 0, 1 });
        detector.setAttackTime (detectorAttackMs);
        detector.setReleaseTime (detectorReleaseMs);

        setAttack (1.0f);
        setRelease (100.0f);

        reset();
    }

    ~Gate() = default;

    void reset()
    {
        detector.reset();

        gain = floorGain;
        holdCounter = 0;
        open = false;
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

            const float levelDb = juce::Decibels::gainToDecibels (detector.processSample (0, sum * scale), silenceDb);

            if (levelDb > thresholdDb)
                open = true;

            // Hold is re-armed anywhere inside the hysteresis band, and only runs down once the signal has left it
            if (levelDb > thresholdDb - hysteresisDb)
                holdCounter = holdSamples;
            else if (holdCounter > 0)
                holdCounter--;
            else
                open = false;

            const float target = open ? 1.0f : floorGain;
            const float cte = target > gain ? attackCoeff : releaseCoeff;

            gain = target + cte * (gain - target);

            for (int j = 0; j < numChannels; j++)
                buffer[j][index] *= gain;
        }
    }

    void setThreshold (float thresholdInDb) { thresholdDb = thresholdInDb; }

    /** How far below the threshold the signal must fall before the gate starts to close. */
    void setHysteresis (float hysteresisInDb) { hysteresisDb = juce::jmax (0.0f, hysteresisInDb); }

    /** The attenuation applied when the gate is shut, so a gate can duck rather than mute. */
    void setRange (float rangeInDb) { floorGain = juce::Decibels::decibelsToGain (rangeInDb); }

    void setHold (float holdInMs) { holdSamples = juce::jmax (0, juce::roundToInt (holdInMs * 1e-3f * sampleRate)); }

    void setAttack (float attackInMs) { attackCoeff = timeToCoefficient (attackInMs); }

    void setRelease (float releaseInMs) { releaseCoeff = timeToCoefficient (releaseInMs); }

    void setMode (Mode m)
    {
        if (m == mode)
            return;

        mode = m;
        detector.setLevelCalculationType (m);
    }

    /** How many dB the last processed sample was attenuated by. */
    float getGainReductionDb() const { return -juce::Decibels::gainToDecibels (gain, silenceDb); }

private:
    static constexpr float silenceDb = -140.0f;
    static constexpr float minTimeMs = 1e-3f;

    // The envelope carries the user's attack and release, so detection only has to be quick and steady
    static constexpr float detectorAttackMs = 0.1f;
    static constexpr float detectorReleaseMs = 10.0f;

    float timeToCoefficient (float timeInMs) const { return std::exp (-1.0f / (juce::jmax (minTimeMs, timeInMs) * 1e-3f * sampleRate)); }

    const float sampleRate;
    const int numChannels;

    float thresholdDb { 0.0f };
    float hysteresisDb { 0.0f };
    float floorGain { 0.0f };
    float attackCoeff { 0.0f };
    float releaseCoeff { 0.0f };
    float gain { 0.0f };
    int holdSamples { 0 };
    int holdCounter { 0 };
    bool open { false };
    Mode mode { Mode::peak };

    juce::dsp::BallisticsFilter<float> detector;

    JUCE_DECLARE_NON_COPYABLE (Gate)
};

} // namespace cgo::dsp
