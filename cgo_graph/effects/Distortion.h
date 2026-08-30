namespace cgo
{

class Distortion : public Processor
{
public:
    struct Params
    {
        ModulatedParameter& drive;
        ModulatedParameter& type;
        ModulatedParameter& output;
        ModulatedParameter& mix;
    } params;

    Distortion();
    ~Distortion() override = default;

    juce::String getTypeId() const override;

private:
    void prepareImpl() override;
    void processImpl (juce::AudioBuffer<float>& buffer) override;

    std::optional<dsp::Distortion> distortion;

    JUCE_DECLARE_NON_COPYABLE (Distortion)
};

} // namespace cgo
