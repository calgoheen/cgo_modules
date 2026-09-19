namespace cgo
{

class TapeStop : public Processor
{
public:
    struct Params
    {
        ModulatedParameter& mode;
        ModulatedParameter& length;
        ModulatedParameter& curve;
        ModulatedParameter& autoBypass;
    } params;

    TapeStop();
    ~TapeStop() override = default;

    juce::String getTypeId() const override;

private:
    void prepareImpl() override;
    void processImpl (juce::AudioBuffer<float>& buffer) override;
    void resetImpl() override;
    void tempoChanged() override;

    void configure();
    void updateParameters();
    void updateLength();

    dsp::TapeStop::Mode lastObservedMode = dsp::TapeStop::Mode::bypass;

    std::optional<dsp::TapeStop> tapeStop;

    JUCE_DECLARE_NON_COPYABLE (TapeStop)
};

} // namespace cgo
