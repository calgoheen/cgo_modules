#include <cgo_dsp/cgo_dsp.h>

namespace cgo::dsp
{

void DryWetFader::prepare (int numChannels, int maxBlockSize, int fadeLengthSamples, int latencySamples)
{
    jassert (numChannels > 0);
    jassert (maxBlockSize > 0);
    jassert (fadeLengthSamples > 0);
    jassert (latencySamples >= 0);

    latency = latencySamples;
    increment = 1.0f / (float) fadeLengthSamples;

    history.setSize (numChannels, maxBlockSize + latency);

    reset();
}

void DryWetFader::reset()
{
    history.clear();

    writePos = 0;
    gain = target;
}

void DryWetFader::setTarget (bool wet) { target = wet ? 1.0f : 0.0f; }

bool DryWetFader::isDry() const { return target <= 0.0f && gain <= 0.0f; }

void DryWetFader::pushDry (const juce::AudioBuffer<float>& input)
{
    const int length = history.getNumSamples();
    const int numSamples = input.getNumSamples();

    jassert (numSamples + latency <= length);

    const int numChannels = juce::jmin (input.getNumChannels(), history.getNumChannels());
    const int untilEnd = juce::jmin (numSamples, length - writePos);

    for (int ch = 0; ch < numChannels; ch++)
    {
        history.copyFrom (ch, writePos, input, ch, 0, untilEnd);
        history.copyFrom (ch, 0, input, ch, untilEnd, numSamples - untilEnd);
    }

    writePos = (writePos + numSamples) % length;
}

void DryWetFader::mix (juce::AudioBuffer<float>& wet)
{
    const int numSamples = wet.getNumSamples();
    const float endGain = advance (gain, numSamples);

    if (gain >= 1.0f && endGain >= 1.0f)
    {
        gain = endGain;
        return;
    }

    const int length = history.getNumSamples();
    const int numChannels = juce::jmin (wet.getNumChannels(), history.getNumChannels());

    int start = writePos - numSamples - latency;

    if (start < 0)
        start += length;

    for (int ch = 0; ch < numChannels; ch++)
    {
        auto* out = wet.getWritePointer (ch);
        const auto* dry = history.getReadPointer (ch);

        float g = gain;
        int read = start;

        for (int i = 0; i < numSamples; i++)
        {
            g = advance (g, 1);
            out[i] = dry[read] + (out[i] - dry[read]) * g;

            read = read + 1 < length ? read + 1 : 0;
        }
    }

    gain = endGain;
}

float DryWetFader::advance (float from, int numSamples) const
{
    const float delta = increment * (float) numSamples;

    return target > from ? juce::jmin (target, from + delta) : juce::jmax (target, from - delta);
}

} // namespace cgo::dsp
