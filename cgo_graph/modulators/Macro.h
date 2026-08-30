namespace cgo
{

class Macro : public Modulator
{
public:
    struct Params
    {
        ModulatedParameter& value;
    } params;

    Macro();

    juce::String getTypeId() const override;

private:
    void prepareImpl() override;
    void processImpl (float* buffer, int numSamples) override;

    JUCE_DECLARE_NON_COPYABLE (Macro)
};

} // namespace cgo
