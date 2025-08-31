namespace cgo
{

class SoundFileUtils
{
public:
    static std::pair<juce::AudioBuffer<float>, double> readWavFile (const juce::File& file)
    {
        jassert (file.hasFileExtension (".wav"));

        juce::WavAudioFormat format;
        auto reader = std::unique_ptr<juce::AudioFormatReader> (format.createReaderFor (new juce::FileInputStream (file), true));

        if (reader == nullptr)
            return { {}, 0.0 };

        return { getAudioDataFromReader (*reader), reader->sampleRate };
    }

    static std::pair<juce::AudioBuffer<float>, double> readWavData (const void* wavData, int wavDataSize)
    {
        juce::WavAudioFormat format;
        std::unique_ptr<juce::AudioFormatReader> reader (format.createReaderFor (new juce::MemoryInputStream (wavData, wavDataSize, false), true));

        if (reader == nullptr)
            return { {}, 0.0 };

        return { getAudioDataFromReader (*reader), reader->sampleRate };
    }

    static void writeWavFile (const juce::AudioBuffer<float>& data, double sampleRate, const juce::File& file)
    {
        jassert (file.hasFileExtension (".wav"));
        file.deleteFile();
        file.create();

        static constexpr juce::uint32 numBits = 16;
        juce::WavAudioFormat format;
        std::unique_ptr<juce::AudioFormatWriter> writer;
        writer.reset (format.createWriterFor (new juce::FileOutputStream (file),
                                              sampleRate,
                                              data.getNumChannels(),
                                              numBits,
                                              {},
                                              0));

        if (writer != nullptr)
            writer->writeFromAudioSampleBuffer (data, 0, data.getNumSamples());
    }

private:
    static juce::AudioBuffer<float> getAudioDataFromReader (juce::AudioFormatReader& reader)
    {
        const int numChannels = static_cast<int> (reader.numChannels);
        const int numSamples = static_cast<int> (reader.lengthInSamples);

        jassert (numSamples == static_cast<int> (reader.lengthInSamples));

        juce::AudioBuffer<float> data;
        data.setSize (numChannels, numSamples);
        reader.read (data.getArrayOfWritePointers(), numChannels, 0, numSamples);

        return std::move (data);
    }
};

} // namespace cgo
