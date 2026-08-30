namespace cgo
{

class Reverb : public Processor
{
public:
    struct Params
    {
        ModulatedParameter& predelay;
        ModulatedParameter& size;
        ModulatedParameter& decay;
        ModulatedParameter& inputCutoff;
        ModulatedParameter& inputBandwidth;
        ModulatedParameter& modulation;
        ModulatedParameter& lowDamping;
        ModulatedParameter& highDamping;
        ModulatedParameter& blend;
        ModulatedParameter& mix;
    } params;

    Reverb();
    ~Reverb() override = default;

    juce::String getTypeId() const override;

private:
    static constexpr int fdnSize = 8;
    static constexpr int diffusionSteps = 4;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void prepareImpl() override;
    void processImpl (juce::AudioBuffer<float>& buffer) override;

    std::optional<dsp::DiffusionReverb<fdnSize, diffusionSteps>> diffusionReverb;

    JUCE_DECLARE_NON_COPYABLE (Reverb)
};

} // namespace cgo
