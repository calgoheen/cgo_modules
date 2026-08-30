#include <cgo_graph/cgo_graph.h>

namespace cgo
{

BasicAudioProcessor::BasicAudioProcessor (const BusesProperties& busesProperties, bool needsMidi) : AudioProcessor (busesProperties), needsMidiInput (needsMidi)
{
}

bool BasicAudioProcessor::acceptsMidi() const { return needsMidiInput; }

void BasicAudioProcessor::releaseResources() {}
void BasicAudioProcessor::getStateInformation (juce::MemoryBlock&) {}
void BasicAudioProcessor::setStateInformation (const void*, int) {}
const juce::String BasicAudioProcessor::getName() const { return ""; }
double BasicAudioProcessor::getTailLengthSeconds() const { return 0.0; }
bool BasicAudioProcessor::producesMidi() const { return false; }
bool BasicAudioProcessor::hasEditor() const { return false; }
juce::AudioProcessorEditor* BasicAudioProcessor::createEditor() { return nullptr; }
int BasicAudioProcessor::getNumPrograms() { return 1; }
int BasicAudioProcessor::getCurrentProgram() { return 0; }
void BasicAudioProcessor::setCurrentProgram (int) {}
const juce::String BasicAudioProcessor::getProgramName (int) { return {}; }
void BasicAudioProcessor::changeProgramName (int, const juce::String&) {}

} // namespace cgo
