namespace cgo
{

class Filter : public Processor
{
public:
    struct Params
    {
        ModulatedParameter& order;
        ModulatedParameter& cutoff;
        ModulatedParameter& resonance;
        ModulatedParameter& type;
    } params;

    Filter();
    ~Filter() override = default;

    juce::String getTypeId() const override;

private:
    void prepareImpl() override;
    void processImpl (juce::AudioBuffer<float>& buffer) override;

    void updateParameters();

    std::optional<dsp::Filter> filter;

    JUCE_DECLARE_NON_COPYABLE (Filter)
};

} // namespace cgo
