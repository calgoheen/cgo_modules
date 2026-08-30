namespace cgo
{

class Phaser : public Processor
{
public:
    struct Params
    {
        ModulatedParameter& notches;
        ModulatedParameter& center;
        ModulatedParameter& spread;
        ModulatedParameter& feedback;
        ModulatedParameter& flipFeedback;
        ModulatedParameter& shape;
        ModulatedParameter& rate;
        ModulatedParameter& amount;
        ModulatedParameter& blend;
        ModulatedParameter& offset;
        ModulatedParameter& safeBass;
        ModulatedParameter& warmth;
        ModulatedParameter& output;
        ModulatedParameter& mix;
    } params;

    Phaser();
    ~Phaser() override = default;

    juce::String getTypeId() const override;

private:
    void prepareImpl() override;
    void processImpl (juce::AudioBuffer<float>& buffer) override;
    void playbackStateChanged() override;
    void tempoChanged() override;
    void playHeadJumped() override;

    void updateRateSync();

    std::optional<dsp::Phaser> phaser;

    JUCE_DECLARE_NON_COPYABLE (Phaser)
};

} // namespace cgo
