namespace cgo
{

class Pitch : public Processor
{
public:
    struct Params
    {
        ModulatedParameter& pitch;
        ModulatedParameter& mix;
    } params;

    Pitch();
    ~Pitch() override = default;

    juce::String getTypeId() const override;

private:
    void prepareImpl() override;
    void processImpl (juce::AudioBuffer<float>& buffer) override;
    void resetImpl() override;

    std::optional<dsp::Pitch> shifter;

    JUCE_DECLARE_NON_COPYABLE (Pitch)
};

} // namespace cgo
