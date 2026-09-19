namespace cgo
{

class Processor : public BasicAudioProcessor, public ParameterOwner, protected PlayHeadState
{
public:
    virtual juce::String getTypeId() const = 0;

    void prepareToPlay (double sampleRate, int samplesPerBlock) final;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) final;
    juce::AudioProcessorParameter* getBypassParameter() const override;

protected:
    template <typename... Params>
    static bool anyChanging (const Params&... params) noexcept
    {
        return (params.isChanging() || ...);
    }

    Processor (const BusesProperties&, bool needsMidiInput = false);

    bool isBusesLayoutSupported (const BusesLayout&) const override;

    virtual void prepareImpl() = 0;
    virtual void processImpl (juce::AudioBuffer<float>&) = 0;

    virtual void resetImpl() {}

    using AudioProcessor::getBlockSize;
    using AudioProcessor::getSampleRate;
    using AudioProcessor::getLatencySamples;
    using AudioProcessor::setLatencySamples;

    juce::dsp::ProcessSpec getProcessSpec() const;
    int getNumChannels() const;

private:
    friend class ModularGraph;

    bool isBypassed() const;
    void upmixMonoInput (juce::AudioBuffer<float>& buffer) const;
    void setActiveModulation (const NodeModulation* modulation);

    ModulatedParameter& bypassParameter;
    dsp::DryWetFader fader;
    bool wasDry = false;
    const NodeModulation* activeModulation = nullptr;

    JUCE_DECLARE_NON_COPYABLE (Processor)
};

} // namespace cgo
