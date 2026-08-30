#include <cgo_graph/cgo_graph.h>

namespace cgo
{

void Processor::prepareToPlay (double sampleRate, int blockSize)
{
    prepareParameters (sampleRate, blockSize);
    prepareImpl();
}

void Processor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    if (auto* playHead = getPlayHead())
        updatePlayHead (*playHead, (double) buffer.getNumSamples() / getSampleRate());

    const auto* modulation = std::exchange (activeModulation, nullptr);

    processParameters (buffer.getNumSamples(), modulation);

    upmixMonoInput (buffer);
    processImpl (buffer);
}

Processor::Processor (const BusesProperties& busesProperties, bool needsMidi) : BasicAudioProcessor (busesProperties, needsMidi) {}

bool Processor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const int numInputs = layouts.getMainInputChannels();
    const int numOutputs = layouts.getMainOutputChannels();

    if (numOutputs != 1 && numOutputs != 2)
        return false;

    return numInputs == numOutputs || (numInputs == 1 && numOutputs == 2);
}

juce::dsp::ProcessSpec Processor::getProcessSpec() const { return { getSampleRate(), (juce::uint32) getBlockSize(), (juce::uint32) getNumChannels() }; }

int Processor::getNumChannels() const { return juce::jmax (getMainBusNumInputChannels(), getMainBusNumOutputChannels()); }

void Processor::upmixMonoInput (juce::AudioBuffer<float>& buffer) const
{
    const int numInputs = getMainBusNumInputChannels();

    if (numInputs != 1)
        return;

    const int numOutputs = juce::jmin (getMainBusNumOutputChannels(), buffer.getNumChannels());

    for (int ch = numInputs; ch < numOutputs; ch++)
        buffer.copyFrom (ch, 0, buffer.getReadPointer (0), buffer.getNumSamples());
}

void Processor::setActiveModulation (const NodeModulation* modulation) { activeModulation = modulation; }

} // namespace cgo
