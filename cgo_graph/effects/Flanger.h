namespace cgo
{

class Flanger : public Processor
{
public:
    struct Params
    {
        ModulatedParameter& delay;
        ModulatedParameter& feedback;
        ModulatedParameter& flipFeedback;
        ModulatedParameter& shape;
        ModulatedParameter& rate;
        ModulatedParameter& depth;
        ModulatedParameter& offset;
        ModulatedParameter& safeBass;
        ModulatedParameter& warmth;
        ModulatedParameter& output;
        ModulatedParameter& mix;
    } params;

    Flanger();
    ~Flanger() override = default;

    juce::String getTypeId() const override;

private:
    void prepareImpl() override;
    void processImpl (juce::AudioBuffer<float>& buffer) override;
    void resetImpl() override;
    void playbackStateChanged() override;
    void tempoChanged() override;
    void playHeadJumped() override;

    void updateRateSync();

    std::optional<dsp::Flanger> flanger;

    JUCE_DECLARE_NON_COPYABLE (Flanger)
};

} // namespace cgo
