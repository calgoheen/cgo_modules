namespace cgo
{

class Random : public Modulator
{
public:
    struct Params
    {
        ModulatedParameter& sync;
        ModulatedParameter& rateFree;
        ModulatedParameter& rateSync;
        ModulatedParameter& smooth;
        ModulatedParameter& steps;
    } params;

    Random();

    juce::String getTypeId() const override;

private:
    void prepareImpl() override;
    void processImpl (float* buffer, int numSamples) override;
    void playbackStateChanged() override;
    void tempoChanged() override;
    void playHeadJumped() override;

    void updateRateSync();
    void step();

    dsp::Phasor phasor;
    juce::Random rng;
    float previousValue { 0.0f };
    float currentValue { 0.0f };
    float lastPhase { 0.0f };

    JUCE_DECLARE_NON_COPYABLE (Random)
};

} // namespace cgo
