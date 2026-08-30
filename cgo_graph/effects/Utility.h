namespace cgo
{

class Utility : public Processor
{
public:
    struct Params
    {
        ModulatedParameter& gain;
        ModulatedParameter& level;
        ModulatedParameter& pan;
        ModulatedParameter& width;
        ModulatedParameter& mono;
        ModulatedParameter& invert;
    } params;

    Utility();
    ~Utility() override = default;

    juce::String getTypeId() const override;

private:
    void prepareImpl() override;
    void processImpl (juce::AudioBuffer<float>& buffer) override;

    void updateParameters();

    std::optional<dsp::Utility> utility;

    JUCE_DECLARE_NON_COPYABLE (Utility)
};

} // namespace cgo
