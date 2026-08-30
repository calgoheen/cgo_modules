namespace cgo
{

class Compressor : public Processor
{
public:
    struct Params
    {
        ModulatedParameter& threshold;
        ModulatedParameter& ratio;
        ModulatedParameter& knee;
        ModulatedParameter& attack;
        ModulatedParameter& release;
        ModulatedParameter& makeup;
        ModulatedParameter& mix;
        ModulatedParameter& mode;
    } params;

    Compressor();
    ~Compressor() override = default;

    juce::String getTypeId() const override;

private:
    void prepareImpl() override;
    void processImpl (juce::AudioBuffer<float>& buffer) override;

    void updateBallistics();

    std::optional<dsp::Compressor> compressor;

    JUCE_DECLARE_NON_COPYABLE (Compressor)
};

} // namespace cgo
