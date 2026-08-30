namespace cgo
{

class BasicAudioProcessor : public juce::AudioProcessor
{
public:
    void prepareToPlay (double sampleRate, int samplesPerBlock) override = 0;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override = 0;

protected:
    BasicAudioProcessor (const BusesProperties&, bool needsMidiInput);
    bool isBusesLayoutSupported (const BusesLayout&) const override = 0;

private:
    bool acceptsMidi() const final;

    const bool needsMidiInput;

    // Unused
    void releaseResources() final;
    void getStateInformation (juce::MemoryBlock&) final;
    void setStateInformation (const void*, int) final;
    const juce::String getName() const final;
    double getTailLengthSeconds() const override;
    bool producesMidi() const final;
    bool hasEditor() const final;
    juce::AudioProcessorEditor* createEditor() final;
    int getNumPrograms() final;
    int getCurrentProgram() final;
    void setCurrentProgram (int) final;
    const juce::String getProgramName (int) final;
    void changeProgramName (int, const juce::String&) final;

    JUCE_DECLARE_NON_COPYABLE (BasicAudioProcessor)
};

} // namespace cgo
