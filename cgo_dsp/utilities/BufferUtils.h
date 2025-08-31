namespace cgo
{

struct BufferUtils
{
    static void applyFade (float* data, int startSample, int numSamples, bool fadeIn);
    static void normalize (juce::AudioBuffer<float>& buffer);
    static juce::AudioBuffer<float> resample (const juce::AudioBuffer<float>& sourceBuffer, double sourceFs, double destFs);
};

} // namespace cgo
