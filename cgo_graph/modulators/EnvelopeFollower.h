namespace cgo
{

class EnvelopeFollower : public Modulator
{
public:
    struct Params
    {
        ModulatedParameter& source;
        ModulatedParameter& attack;
        ModulatedParameter& release;
        ModulatedParameter& gain;
        ModulatedParameter& range;
        ModulatedParameter& mode;
    } params;

    EnvelopeFollower();

    juce::String getTypeId() const override;

private:
    using Mode = juce::dsp::BallisticsFilterLevelCalculationType;

    enum Source
    {
        input = 0,
        sidechain
    };

    static constexpr float silenceDb = -140.0f;

    void prepareImpl() override;
    void processImpl (float* buffer, int numSamples) override;

    void updateBallistics();

    Mode mode { Mode::peak };

    juce::dsp::BallisticsFilter<float> detector;

    JUCE_DECLARE_NON_COPYABLE (EnvelopeFollower)
};

} // namespace cgo
