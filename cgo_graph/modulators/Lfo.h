namespace cgo
{

class Lfo : public Modulator
{
public:
    struct Params
    {
        ModulatedParameter& shape;
        ModulatedParameter& sync;
        ModulatedParameter& rateFree;
        ModulatedParameter& rateSync;
        ModulatedParameter& phase;
    } params;

    Lfo();

    juce::String getTypeId() const override;

private:
    void prepareImpl() override;
    void processImpl (float* buffer, int numSamples) override;
    void playbackStateChanged() override;
    void tempoChanged() override;
    void playHeadJumped() override;

    void updateRateSync();

    dsp::Phasor phasor;

    JUCE_DECLARE_NON_COPYABLE (Lfo)
};

} // namespace cgo
