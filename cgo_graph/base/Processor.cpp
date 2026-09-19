#include <cgo_graph/cgo_graph.h>

namespace cgo
{

constexpr double bypassFadeSeconds = 10e-3;

void Processor::prepareToPlay (double sampleRate, int blockSize)
{
    prepareParameters (sampleRate, blockSize);
    prepareImpl();

    wasDry = isBypassed();

    fader.setTarget (! isBypassed());
    fader.prepare (getNumChannels(), blockSize, juce::jmax (1, juce::roundToInt (sampleRate * bypassFadeSeconds)), getLatencySamples());
}

void Processor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    if (auto* playHead = getPlayHead())
        updatePlayHead (*playHead, (double) buffer.getNumSamples() / getSampleRate());

    const auto* modulation = std::exchange (activeModulation, nullptr);

    fader.setTarget (! isBypassed());

    const bool dry = fader.isDry();

    // Nothing pushed the parameters to the DSP while processImpl was skipped, so resuming has
    // to replay everything that changed in the meantime
    if (wasDry && ! dry)
        markParametersUnsettled();

    processParameters (buffer.getNumSamples(), modulation);

    upmixMonoInput (buffer);

    fader.pushDry (buffer);

    if (dry && ! wasDry)
        resetImpl();

    if (! dry)
        processImpl (buffer);

    wasDry = dry;

    fader.mix (buffer);
}

juce::AudioProcessorParameter* Processor::getBypassParameter() const { return &bypassParameter.parameter; }

Processor::Processor (const BusesProperties& busesProperties, bool needsMidi)
  : BasicAudioProcessor (busesProperties, needsMidi), bypassParameter (addModulatedParameter (ParamUtils::createBoolParameter ("bypass", "Bypass", false)))
{
}

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

bool Processor::isBypassed() const { return bypassParameter.parameter.getValue() >= 0.5f; }

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
