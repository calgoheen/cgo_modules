#include <cgo_graph/cgo_graph.h>

namespace cgo
{

namespace graphIds
{

const juce::Identifier GRAPH { "GRAPH" };

} // namespace graphIds

constexpr size_t midiScratchBytes = 512;

ModularGraph::ModularGraph (bool isSynth, int numSidechainChannels, int subBlock)
  : BasicAudioProcessor (getBusesProperties (isSynth, numSidechainChannels), true),
    subBlockSize (juce::jmax (1, subBlock)),
    processorGraph (getMainBusesLayout (numSidechainChannels)),
    modulationGraph (processorGraph, modulatorRegistry)
{
    jassert (numSidechainChannels == 0 || ! isSynth);

    processorGraph.setPlayHead (&subBlockPlayHead);
    modulatorRegistry.setPlayHead (&subBlockPlayHead);
    modulationGraph.compile();
}

void ModularGraph::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused (samplesPerBlock);

    modulatorRegistry.prepare (sampleRate, subBlockSize);
    modulationGraph.prepare (sampleRate, subBlockSize);
    processorGraph.prepare (sampleRate, subBlockSize);

    subBlockPlayHead.prepare (sampleRate);
    subBlockMidi.ensureSize (midiScratchBytes);
}

void ModularGraph::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    auto* snapshot = modulationGraph.loadSnapshot();

    auto* const* channels = buffer.getArrayOfWritePointers();
    const int totalChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();

    const int mainChannels = juce::jmin (getMainBusNumOutputChannels(), totalChannels);
    const int sidechainChannels = totalChannels - mainChannels;

    for (int start = 0; start < numSamples; start += subBlockSize)
    {
        const int numToProcess = juce::jmin (subBlockSize, numSamples - start);

        juce::AudioBuffer<float> subBlock (channels, mainChannels, start, numToProcess);
        juce::AudioBuffer<float> sidechain (channels + mainChannels, sidechainChannels, start, numToProcess);

        subBlockMidi.clear();
        subBlockMidi.addEvents (midi, start, numToProcess, -start);
        subBlockPlayHead.setOffset (start);

        if (snapshot != nullptr)
        {
            const ModulatorAudio audio { &subBlock, sidechainChannels > 0 ? &sidechain : nullptr };

            for (auto& entry : snapshot->modulators)
                entry.modulator->process (numToProcess, &entry.modulation, audio);

            for (auto& entry : snapshot->processors)
                static_cast<Processor*> (entry.node->getProcessor())->setActiveModulation (&entry.modulation);
        }

        processorGraph.processBlock (subBlock, subBlockMidi);
    }

    midi.clear();
}

void ModularGraph::setPlayHead (juce::AudioPlayHead* newPlayHead)
{
    juce::AudioProcessor::setPlayHead (newPlayHead);
    subBlockPlayHead.setSource (newPlayHead);
}

bool ModularGraph::contains (NodeRef node) const
{
    return std::visit (
        [this] (auto id)
        {
            if constexpr (std::is_same_v<std::decay_t<decltype (id)>, ProcessorID>)
                return processorGraph.contains (id);
            else
                return modulatorRegistry.contains (id);
        },
        node);
}

ParameterOwner& ModularGraph::getNode (NodeRef node) const
{
    return std::visit (
        [this] (auto id) -> ParameterOwner&
        {
            if constexpr (std::is_same_v<std::decay_t<decltype (id)>, ProcessorID>)
                return processorGraph.getProcessor (id);
            else
                return modulatorRegistry.getModulator (id);
        },
        node);
}

ModulatedParameter* ModularGraph::findModulatedParameter (NodeRef node, int index) const
{
    if (! contains (node))
        return nullptr;

    const auto& params = getNode (node).getModulatedParameters();

    return juce::isPositiveAndBelow (index, params.size()) ? params[index] : nullptr;
}

ModulatorID ModularGraph::addModulator (std::unique_ptr<Modulator> modulator, std::optional<ModulatorID> explicitId)
{
    const auto id = modulatorRegistry.addModulator (std::move (modulator), explicitId);
    modulationGraph.compile();

    return id;
}

void ModularGraph::removeModulator (ModulatorID id)
{
    modulationGraph.removeModulationsReferencing (id);
    modulatorRegistry.removeModulator (id);
    modulationGraph.compile();
}

ProcessorID ModularGraph::addProcessor (std::unique_ptr<Processor> processor, std::optional<ProcessorID> explicitId)
{
    const auto id = processorGraph.addProcessor (std::move (processor), explicitId);
    modulationGraph.compile();

    return id;
}

void ModularGraph::removeProcessor (ProcessorID id)
{
    modulationGraph.removeModulationsReferencing (id);
    processorGraph.removeProcessor (id);
    modulationGraph.compile();
}

juce::ValueTree ModularGraph::toValueTree() const
{
    juce::ValueTree tree { graphIds::GRAPH };

    tree.appendChild (modulatorRegistry.toValueTree(), nullptr);
    tree.appendChild (processorGraph.toValueTree(), nullptr);
    tree.appendChild (modulationGraph.toValueTree(), nullptr);

    return tree;
}

void ModularGraph::restoreFromValueTree (const juce::ValueTree& parent)
{
    jassert (modulatorRegistry.getAll().empty() && processorGraph.getAll().empty());

    const auto tree = parent.getChildWithName (graphIds::GRAPH);

    modulatorRegistry.restoreFromValueTree (tree);
    processorGraph.restoreFromValueTree (tree);
    modulationGraph.restoreFromValueTree (tree);

    modulationGraph.compile();
}

juce::Optional<juce::AudioPlayHead::PositionInfo> ModularGraph::SubBlockPlayHead::getPosition() const
{
    if (source == nullptr)
        return juce::nullopt;

    auto position = source->getPosition();

    if (offset == 0 || sampleRate <= 0.0 || ! position.hasValue())
        return position;

    const double offsetSeconds = (double) offset / sampleRate;

    if (const auto timeInSamples = position->getTimeInSamples())
        position->setTimeInSamples (*timeInSamples + offset);

    if (const auto timeInSeconds = position->getTimeInSeconds())
        position->setTimeInSeconds (*timeInSeconds + offsetSeconds);

    // The last bar start is a fixed point in the timeline, so it is deliberately left alone.
    if (const auto ppqPosition = position->getPpqPosition())
        if (const auto bpm = position->getBpm())
            position->setPpqPosition (*ppqPosition + offsetSeconds * *bpm / 60.0);

    return position;
}

bool ModularGraph::SubBlockPlayHead::canControlTransport() { return source != nullptr && source->canControlTransport(); }

void ModularGraph::SubBlockPlayHead::transportPlay (bool shouldStartPlaying)
{
    if (source != nullptr)
        source->transportPlay (shouldStartPlaying);
}

void ModularGraph::SubBlockPlayHead::transportRecord (bool shouldStartRecording)
{
    if (source != nullptr)
        source->transportRecord (shouldStartRecording);
}

void ModularGraph::SubBlockPlayHead::transportRewind()
{
    if (source != nullptr)
        source->transportRewind();
}

void ModularGraph::SubBlockPlayHead::setSource (juce::AudioPlayHead* newSource) { source = newSource; }

void ModularGraph::SubBlockPlayHead::prepare (double newSampleRate) { sampleRate = newSampleRate; }

void ModularGraph::SubBlockPlayHead::setOffset (int startSample) { offset = startSample; }

ModularGraph::BusesProperties ModularGraph::getBusesProperties (bool isSynth, int numSidechainChannels)
{
    auto props = BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo());

    if (! isSynth)
        props.addBus (true, "Input", juce::AudioChannelSet::stereo());

    if (numSidechainChannels > 0)
        props.addBus (true, "Sidechain", juce::AudioChannelSet::discreteChannels (numSidechainChannels));

    return props;
}

ModularGraph::BusesLayout ModularGraph::getMainBusesLayout (int numSidechainChannels) const
{
    auto layout = getBusesLayout();

    // The sidechain reaches the modulators directly, so the processor chain never sees it. It is
    // the last input bus getBusesProperties added, and the only one at all when isSynth.
    if (numSidechainChannels > 0)
        layout.inputBuses.removeLast();

    return layout;
}

bool ModularGraph::isBusesLayoutSupported (const BusesLayout& layout) const { return layout == getBusesLayout(); }

} // namespace cgo
