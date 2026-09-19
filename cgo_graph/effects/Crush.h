namespace cgo
{

class Crush : public Processor
{
public:
    struct Params
    {
        ModulatedParameter& downsample;
        ModulatedParameter& bitDepth;
    } params;

    Crush();
    ~Crush() override = default;

    juce::String getTypeId() const override;

private:
    void prepareImpl() override;
    void processImpl (juce::AudioBuffer<float>& buffer) override;
    void resetImpl() override;

    std::optional<dsp::Crush> crush;

    JUCE_DECLARE_NON_COPYABLE (Crush)
};

} // namespace cgo
