#include <cgo_processors/cgo_processors.h>

namespace cgo
{

void Processor::prepareToPlay (double sampleRate, int samplesPerBlock, int numChannels)
{
    currentSampleRate = sampleRate;
    currentBlockSize = samplesPerBlock;
    currentNumChannels = numChannels;

    parameterUpdater.flush();
    reset();
}

void Processor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&, juce::AudioPlayHead* playHead)
{
    if (playHead)
        updatePlayHead (*playHead);

    parameterUpdater.update();
    process (buffer.getArrayOfWritePointers(), 0, buffer.getNumSamples());
}

double Processor::getLatencySamples() const
{
    return latencySamples;
}

juce::dsp::ProcessSpec Processor::getProcessSpec() const
{
    return {
        currentSampleRate,
        static_cast<juce::uint32> (currentBlockSize),
        static_cast<juce::uint32> (currentNumChannels)
    };
}

double Processor::getSampleRate() const
{
    return currentSampleRate;
}

int Processor::getBlockSize() const
{
    return currentBlockSize;
}

int Processor::getNumChannels() const
{
    return currentNumChannels;
}

bool Processor::getPlaybackState() const
{
    return playbackState;
}

double Processor::getPlaybackPosition() const
{
    return ppq;
}

double Processor::getSecondsPerBeat() const
{
    return 60.0 / tempo;
}

void Processor::setLatencySamples (double latency)
{
    latencySamples = latency;
}

void Processor::updatePlayHead (juce::AudioPlayHead& ph)
{
    if (const auto pos = ph.getPosition())
    {
        const double nextPpq = pos->getPpqPosition().orFallback (ppq);
        if (ppq != nextPpq)
            ppq = nextPpq;

        const bool nextPlaybackState = pos->getIsPlaying();
        if (playbackState != nextPlaybackState)
        {
            playbackState = nextPlaybackState;
            playbackStateChanged();
        }

        const double nextTempo = pos->getBpm().orFallback (tempo);
        if (tempo != nextTempo)
        {
            tempo = nextTempo;
            tempoChanged();
        }
    }
}

} // namespace cgo
