namespace cgo
{

class Gate : public Processor
{
public:
    struct Params
    {
        ModulatedParameter& threshold;
        ModulatedParameter& hysteresis;
        ModulatedParameter& range;
        ModulatedParameter& attack;
        ModulatedParameter& hold;
        ModulatedParameter& release;
        ModulatedParameter& mode;
    } params;

    Gate();
    ~Gate() override = default;

    juce::String getTypeId() const override;

private:
    void prepareImpl() override;
    void processImpl (juce::AudioBuffer<float>& buffer) override;

    void updateBallistics();

    std::optional<dsp::Gate> gate;

    JUCE_DECLARE_NON_COPYABLE (Gate)
};

} // namespace cgo
