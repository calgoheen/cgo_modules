namespace cgo
{

class Delay : public Processor
{
public:
    struct Params
    {
        ModulatedParameter& delayLeft;
        ModulatedParameter& delayRight;
        ModulatedParameter& glide;
        ModulatedParameter& mode;
        ModulatedParameter& feedback;
        ModulatedParameter& centerFrequency;
        ModulatedParameter& bandwidth;
        ModulatedParameter& output;
        ModulatedParameter& mix;
    } params;

    Delay();
    ~Delay() override = default;

    juce::String getTypeId() const override;

private:
    void prepareImpl() override;
    void processImpl (juce::AudioBuffer<float>& buffer) override;
    void resetImpl() override;
    void tempoChanged() override;

    void updateDelayTimes();

    std::optional<dsp::Delay> delay;

    JUCE_DECLARE_NON_COPYABLE (Delay)
};

} // namespace cgo
