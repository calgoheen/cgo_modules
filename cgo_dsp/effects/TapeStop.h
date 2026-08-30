namespace cgo::dsp
{

class TapeStop
{
public:
    enum class Mode
    {
        bypass = 0,
        stop,
        start
    };

    enum class FilterType
    {
        lowpass = 0,
        highpass,
        bandpass
    };

    static constexpr float maxDelaySeconds = 20.0f;

    TapeStop (float sr, int numChans) : sampleRate (sr), numChannels (numChans), delay ((int) (maxDelaySeconds * sampleRate) + 10)
    {
        delay.prepare ({ 0.0, 0, (juce::uint32) numChannels });

        for (auto& f : filters)
            f.prepare ({ (double) sampleRate, 0, (juce::uint32) numChannels });

        crossfadeBuffer.setSize (numChannels, 1);

        setFilterType (FilterType::lowpass);
        setFilter (22e3f, 0.71f);

        reset();
    }

    ~TapeStop() = default;

    void reset()
    {
        delay.reset();

        for (auto& f : filters)
            f.reset();

        crossfadeBuffer.clear();

        prevSettings = {};
        currentSettings = {};

        prevSettings.filter = &filters[0];
        currentSettings.filter = &filters[1];
    }

    void process (float* const* buffer, int startSample, int numSamples)
    {
        const auto crossfadeBufferPtr = crossfadeBuffer.getArrayOfWritePointers();

        for (int n = 0; n < numSamples; n++)
        {
            const int index = startSample + n;

            for (int j = 0; j < numChannels; j++)
            {
                delay.pushSample (j, buffer[j][index]);
                crossfadeBufferPtr[j][0] = buffer[j][index];
            }

            processSettings (buffer, index, currentSettings);

            if (currentSettings.counter <= currentSettings.crossfadeLength)
            {
                processSettings (crossfadeBufferPtr, 0, prevSettings);

                const float mix = (float) currentSettings.counter / (float) currentSettings.crossfadeLength;
                const auto [prevGain, currentGain] = AudioUtils::constantPowerMix (mix);

                for (int j = 0; j < numChannels; j++)
                    buffer[j][index] = buffer[j][index] * currentGain + crossfadeBufferPtr[j][0] * prevGain;
            }

            for (int j = 0; j < numChannels; j++)
                delay.incrementReadPointer (j);

            if (autoBypass && currentSettings.mode != Mode::bypass && currentSettings.length - currentSettings.counter < crossfadeLengthSamples / 2)
                updateMode (Mode::bypass);
        }
    }

    Mode getMode() const { return currentSettings.mode; }

    /** Re-arms the transport, crossfading out of whatever it was doing. */
    void setMode (Mode nextMode, bool startCompleted = false)
    {
        updateMode (nextMode);

        if (startCompleted)
            currentSettings.counter = currentSettings.length;
    }

    void setAutoBypass (bool shouldAutoBypass) { autoBypass = shouldAutoBypass; }

    void setSlowdownLength (float lengthInSeconds) { slowdownLengthSamples = juce::roundToInt (juce::jmin (lengthInSeconds, maxDelaySeconds) * sampleRate); }

    void setSpeedupLength (float lengthInSeconds) { speedupLengthSamples = juce::roundToInt (juce::jmin (lengthInSeconds, maxDelaySeconds) * sampleRate); }

    void setSlowdownCurve (float amount) { slowdownCurve = juce::jmap (amount, -1.0f, 1.0f, -0.5f, 0.5f); }

    void setSpeedupCurve (float amount) { speedupCurve = juce::jmap (-amount, -1.0f, 1.0f, -0.5f, 0.5f); }

    void setSlowdownRange (float start, float end)
    {
        slowdownStart = juce::jmin (start, end - minRangeSeparation);
        slowdownEnd = juce::jmax (end, start + minRangeSeparation);
    }

    void setSpeedupRange (float start, float end)
    {
        speedupStart = juce::jmin (start, end - minRangeSeparation);
        speedupEnd = juce::jmax (end, start + minRangeSeparation);
    }

    void setFadeLength (float lengthInSeconds) { fadeLengthSamples = juce::roundToInt (lengthInSeconds * sampleRate); }

    void setCrossfadeLength (float lengthInSeconds) { crossfadeLengthSamples = juce::roundToInt (lengthInSeconds * sampleRate); }

    void setFilterType (FilterType filterType)
    {
        const float mode = filterType == FilterType::lowpass ? 0.0f : filterType == FilterType::highpass ? 1.0f : 0.5f;

        for (auto& f : filters)
            f.setMode (mode);
    }

    void setFilter (float cutoffHz, float resonance)
    {
        for (auto& f : filters)
        {
            f.setCutoffFrequency<false> (cutoffHz);
            f.setQValue<true> (resonance);
        }
    }

private:
    using DelayLine = chowdsp::DelayLine<float, chowdsp::DelayLineInterpolationTypes::Lagrange5th>;
    using Filter = chowdsp::StateVariableFilter<float, chowdsp::StateVariableFilterType::MultiMode>;

    struct Settings
    {
        Mode mode = Mode::bypass;

        int counter = 0;
        int length = 0;
        int fadeLength = 0;
        int crossfadeLength = 0;

        double delay = 0.0;
        double curve = 0.0;
        double start = 0.0;
        double end = 1.0;

        Filter* filter = nullptr;
    };

    static constexpr float minRangeSeparation = 0.1f;

    void processSettings (float* const* buffer, int index, Settings& settings)
    {
        if (settings.mode != Mode::bypass)
        {
            if (settings.counter < settings.length)
            {
                const double position = (double) (settings.counter + 1) / (double) settings.length;
                const double mapped = Curve::exponential (juce::jmap (position, settings.start, settings.end), settings.curve);

                settings.delay += settings.mode == Mode::stop ? mapped : 1.0 - mapped;
            }

            const float fadeGain = [&settings]
            {
                const int fadePosition = settings.mode == Mode::stop ? (settings.length - settings.counter) : settings.counter;

                if (fadePosition < settings.fadeLength)
                    return std::pow ((float) fadePosition / (float) settings.fadeLength, 2.5f);

                return 1.0f;
            }();

            for (int j = 0; j < numChannels; j++)
            {
                const float delayOut = delay.popSample (j, (float) settings.delay, false);
                const float filterOut = settings.filter->processSample (j, delayOut);
                buffer[j][index] = fadeGain * filterOut;
            }
        }

        if (settings.counter < settings.length)
            ++settings.counter;
    }

    void updateMode (Mode nextMode)
    {
        const auto nextFilterToUse = prevSettings.filter;
        prevSettings = currentSettings;

        currentSettings.mode = nextMode;
        currentSettings.counter = 0;
        currentSettings.delay = 0.0;

        if (nextMode == Mode::bypass)
        {
            currentSettings.length = crossfadeLengthSamples;
        }
        else if (nextMode == Mode::stop)
        {
            currentSettings.length = slowdownLengthSamples;
            currentSettings.curve = slowdownCurve;
            currentSettings.start = slowdownStart;
            currentSettings.end = slowdownEnd;
        }
        else
        {
            currentSettings.length = speedupLengthSamples;
            currentSettings.curve = speedupCurve;
            currentSettings.start = speedupStart;
            currentSettings.end = speedupEnd;
        }

        currentSettings.fadeLength = juce::jmin (currentSettings.length, fadeLengthSamples);
        currentSettings.crossfadeLength = juce::jmin (currentSettings.length, crossfadeLengthSamples);

        nextFilterToUse->reset();
        currentSettings.filter = nextFilterToUse;
    }

    const float sampleRate;
    const int numChannels;

    bool autoBypass { false };

    int slowdownLengthSamples { 0 };
    int speedupLengthSamples { 0 };
    double slowdownCurve { 0.0 };
    double speedupCurve { 0.0 };
    double slowdownStart { 0.0 };
    double speedupStart { 0.0 };
    double slowdownEnd { 1.0 };
    double speedupEnd { 1.0 };
    int fadeLengthSamples { 0 };
    int crossfadeLengthSamples { 0 };

    Settings currentSettings, prevSettings;

    juce::AudioBuffer<float> crossfadeBuffer;

    DelayLine delay;
    std::array<Filter, 2> filters;

    JUCE_DECLARE_NON_COPYABLE (TapeStop)
};

} // namespace cgo::dsp
