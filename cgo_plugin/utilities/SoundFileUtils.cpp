#include <cgo_plugin/cgo_plugin.h>

namespace cgo::SoundFileUtils
{

CGO_ANON_NAMESPACE_BEGIN

juce::AudioBuffer<float> getAudioDataFromReader (juce::AudioFormatReader& reader)
{
    const int numChannels = (int) reader.numChannels;
    const int numSamples = (int) reader.lengthInSamples;

    jassert (numSamples == (int) reader.lengthInSamples);

    juce::AudioBuffer<float> data;
    data.setSize (numChannels, numSamples);
    reader.read (data.getArrayOfWritePointers(), numChannels, 0, numSamples);

    return data;
}

CGO_ANON_NAMESPACE_END

std::pair<juce::AudioBuffer<float>, double> readWavFile (const juce::File& file)
{
    jassert (file.hasFileExtension (".wav"));

    juce::WavAudioFormat format;
    auto reader = std::unique_ptr<juce::AudioFormatReader> (format.createReaderFor (new juce::FileInputStream (file), true));

    if (reader == nullptr)
        return { {}, 0.0 };

    return { ANON::getAudioDataFromReader (*reader), reader->sampleRate };
}

std::pair<juce::AudioBuffer<float>, double> readWavData (const void* wavData, int wavDataSize)
{
    juce::WavAudioFormat format;
    std::unique_ptr<juce::AudioFormatReader> reader (format.createReaderFor (new juce::MemoryInputStream (wavData, (size_t) wavDataSize, false), true));

    if (reader == nullptr)
        return { {}, 0.0 };

    return { ANON::getAudioDataFromReader (*reader), reader->sampleRate };
}

void writeWavFile (const juce::AudioBuffer<float>& data, double sampleRate, const juce::File& file)
{
    jassert (file.hasFileExtension (".wav"));
    file.deleteFile();
    file.create();

    constexpr juce::uint32 numBits = 16;
    juce::WavAudioFormat format;
    std::unique_ptr<juce::AudioFormatWriter> writer;
    writer.reset (format.createWriterFor (new juce::FileOutputStream (file), sampleRate, (unsigned int) data.getNumChannels(), numBits, {}, 0));

    if (writer != nullptr)
        writer->writeFromAudioSampleBuffer (data, 0, data.getNumSamples());
}

} // namespace cgo::SoundFileUtils
