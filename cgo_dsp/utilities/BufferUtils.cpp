#include <cgo_dsp/cgo_dsp.h>

namespace cgo
{

void BufferUtils::applyFade (float* data, int startSample, int numSamples, bool fadeIn)
{
    if (fadeIn)
    {
        for (int i = 0; i < numSamples; i++)
            data[startSample + i] *= std::sqrt (static_cast<float> (i) / numSamples);
    }
    else
    {
        for (int i = 0; i < numSamples; i++)
            data[startSample + i] *= std::sqrt (static_cast<float> (numSamples - i) / numSamples);
    }
}

void BufferUtils::normalize (juce::AudioBuffer<float>& buffer)
{
    auto ptr = buffer.getArrayOfWritePointers();

    float max = 0.0f;
    for (int j = 0; j < buffer.getNumChannels(); j++)
        for (int i = 0; i < buffer.getNumSamples(); i++)
            max = juce::jmax (max, std::abs (ptr[j][i]));

    for (int j = 0; j < buffer.getNumChannels(); j++)
        for (int i = 0; i < buffer.getNumSamples(); i++)
            ptr[j][i] /= max;
}

juce::AudioBuffer<float> BufferUtils::resample (const juce::AudioBuffer<float>& sourceBuffer, double sourceFs, double destFs)
{
    int numChannels = sourceBuffer.getNumChannels();
    int sourceLength = sourceBuffer.getNumSamples();
    int destLength = std::floor (sourceLength * destFs / sourceFs);

    juce::AudioBuffer<float> outBuffer;
    outBuffer.setSize (numChannels, destLength);

    for (int j = 0; j < numChannels; j++)
    {
        auto resampler = std::make_unique<r8b::CDSPResampler> (sourceFs, destFs, sourceLength);
        auto numRequired = resampler->getInputRequiredForOutput (destLength);

        juce::AudioBuffer<double> sourceBufferDouble;
        sourceBufferDouble.setSize (1, numRequired);
        sourceBufferDouble.clear();

        for (int i = 0; i < sourceLength; i++)
            sourceBufferDouble.setSample (0, i, static_cast<double> (sourceBuffer.getSample (j, i)));

        // Do resample and save to float output buffer
        double* outPtr;
        resampler->process (sourceBufferDouble.getWritePointer (0), numRequired, outPtr);
        for (int i = 0; i < destLength; i++)
            outBuffer.setSample (j, i, static_cast<float> (outPtr[i]));
    }

    return std::move (outBuffer);
}

} // namespace cgo
