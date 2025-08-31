namespace cgo
{

class Fifo
{
public:
    Fifo() = default;

    void prepare (int numChannels, int maxNumSamples)
    {
        jassert (numChannels > 0);
        buffer.setSize (numChannels, maxNumSamples);

        for (int i = 0; i < numChannels; i++)
            abstractFifos.add (new juce::SingleThreadedAbstractFifo (maxNumSamples));
    }

    void write (int channel, float sample)
    {
        jassert (getNumAvailableToWrite (channel) >= 1);

        auto ranges = abstractFifos[channel]->write (1);
        buffer.setSample (channel, ranges[0].getStart(), sample);
    }

    float read (int channel)
    {
        jassert (getNumAvailableToRead (channel) >= 1);

        auto ranges = abstractFifos[channel]->read (1);
        return buffer.getSample (channel, ranges[0].getStart());
    }

    int getNumAvailableToWrite (int channel) const
    {
        jassert (channel < buffer.getNumChannels());

        return abstractFifos[channel]->getRemainingSpace();
    }

    int getNumAvailableToRead (int channel) const
    {
        jassert (channel < buffer.getNumChannels());

        return abstractFifos[channel]->getNumReadable();
    }

private:
    juce::OwnedArray<juce::SingleThreadedAbstractFifo> abstractFifos;
    juce::AudioBuffer<float> buffer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Fifo)
};

} // namespace cgo
