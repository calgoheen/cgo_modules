namespace cgo::dsp::BufferUtils
{

void applyFade (float* data, int startSample, int numSamples, bool fadeIn);
void normalize (juce::AudioBuffer<float>& buffer);
juce::AudioBuffer<float> resample (const juce::AudioBuffer<float>& sourceBuffer, double sourceFs, double destFs);

} // namespace cgo::dsp::BufferUtils
