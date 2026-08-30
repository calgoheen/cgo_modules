namespace cgo::SoundFileUtils
{

std::pair<juce::AudioBuffer<float>, double> readWavFile (const juce::File& file);
std::pair<juce::AudioBuffer<float>, double> readWavData (const void* wavData, int wavDataSize);
void writeWavFile (const juce::AudioBuffer<float>& data, double sampleRate, const juce::File& file);

} // namespace cgo::SoundFileUtils
