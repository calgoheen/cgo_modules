#include <cgo_graph/cgo_graph.h>

namespace cgo
{

void Modulator::prepare (double sr, int maxBlockSize)
{
    sampleRate = sr;
    blockSize = maxBlockSize;

    buffer.prepare (maxBlockSize);
    prepareParameters (sr, maxBlockSize);
    prepareImpl();
}

void Modulator::process (int numSamples, const NodeModulation* modulation, const ModulatorAudio& newAudio)
{
    if (playHead != nullptr)
        updatePlayHead (*playHead, (double) numSamples / sampleRate);

    audio = newAudio;

    processParameters (numSamples, modulation);
    const auto block = buffer.write (numSamples);
    processImpl (block.data(), numSamples);
}

void Modulator::setPlayHead (juce::AudioPlayHead* newPlayHead) { playHead = newPlayHead; }

double Modulator::getSampleRate() const { return sampleRate; }
int Modulator::getBlockSize() const { return blockSize; }
const ModulatorAudio& Modulator::getAudio() const { return audio; }

} // namespace cgo
