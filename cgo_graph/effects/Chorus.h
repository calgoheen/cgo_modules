namespace cgo
{

class Chorus : public Processor
{
public:
    struct Params
    {
        ModulatedParameter& type;
        ModulatedParameter& rate;
        ModulatedParameter& amount;
        ModulatedParameter& feedback;
        ModulatedParameter& flipFeedback;
        ModulatedParameter& width;
        ModulatedParameter& warmth;
        ModulatedParameter& output;
        ModulatedParameter& mix;
    } params;

    Chorus();
    ~Chorus() override = default;

    juce::String getTypeId() const override;

private:
    void prepareImpl() override;
    void processImpl (juce::AudioBuffer<float>& buffer) override;
    void resetImpl() override;
    void playbackStateChanged() override;

    std::optional<dsp::Chorus> chorus;

    JUCE_DECLARE_NON_COPYABLE (Chorus)
};

} // namespace cgo
