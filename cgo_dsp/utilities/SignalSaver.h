namespace cgo
{

template <int N, int L>
class SignalSaver : public juce::Timer
{
public:
    SignalSaver (int timerLengthSeconds = 10,
                 const juce::File& outputDir = juce::File::getSpecialLocation (juce::File::userDesktopDirectory))
        : dest (outputDir)
    {
        for (int i = 0; i < N; i++)
        {
            std::fill (buffers[i].begin(), buffers[i].end(), 0.0f);
            iterators[i].setLength (L);
        }

        startTimer (timerLengthSeconds * 1000);
    }

    ~SignalSaver() override = default;

protected:
    void push (int channel, float x)
    {
        jassert (channel < N);

        juce::ScopedLock sl (lock);
        const int idx = iterators[channel]();
        buffers[channel][idx] = x;
    }

    void save()
    {
        {
            juce::ScopedLock sl (lock);
            savedBuffers = buffers;
            savedIterators = iterators;
        }

        for (int i = 0; i < N; i++)
            saveChannel (i);
    }

private:
    void timerCallback() override
    {
        save();
    }

    void saveChannel (int channel)
    {
        juce::AudioBuffer<float> temp (1, L);
        for (int i = 0; i < L; i++)
        {
            const int idx = savedIterators[channel].get (i);
            temp.setSample (0, i, savedBuffers[channel][idx]);
        }

        auto file = dest.getChildFile ("channel_" + juce::String (channel) + ".wav");
        file.deleteFile();

        juce::WavAudioFormat format;
        auto writer = std::unique_ptr<juce::AudioFormatWriter> (format.createWriterFor (file.createOutputStream().release(), 48e3, 1, 16, {}, 0));
        writer->writeFromAudioSampleBuffer (temp, 0, L);
    }

    const juce::File dest;

    using Buffers = std::array<std::array<float, L>, N>;
    using Iterators = std::array<CircularIterator, N>;

    Buffers buffers, savedBuffers;
    Iterators iterators, savedIterators;

    juce::CriticalSection lock;
};

} // namespace cgo
