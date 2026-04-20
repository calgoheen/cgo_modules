namespace cgo
{

class Reverb : public Processor
{
public:
    struct Params
    {
        Params()
        {
            predelay = ParamUtils::createTimeParameter ("predelay", "Predelay", 0.0005f, 0.25f, 0.05f, 0.0005f);
            size = ParamUtils::createTimeParameter ("size", "Size", 0.01f, maxSizeSeconds, 0.1f, 0.1f);
            decay = ParamUtils::createTimeParameter ("decay", "Decay", 0.01f, 20.0f, 2.0f, 2.0f);
            inputFilterLowpassCutoff = ParamUtils::createFreqParameter ("iflp_cut", "Input LP Cutoff", 20.0f, 20e3f, 1e3f, 22e3f);
            inputFilterHighpassCutoff = ParamUtils::createFreqParameter ("ifhp_cut", "Input HP Cutoff", 20.0f, 20e3f, 1e3f, 20.0f);
            diffusionModRate = ParamUtils::createFreqParameter ("diff_mod_rate", "Diffusion Mod Rate", 0.01f, 5.0f, 0.5f, 0.5f);
            diffusionModDepth = ParamUtils::createPercentParameter ("diff_mod_depth", "Diffusion Mod Depth", 0.2f);
            tailFilterLowCutoff = ParamUtils::createFreqParameter ("tls_cut", "Tail LS Cutoff", 20.0f, 20e3f, 1e3f, 250.0f);
            tailFilterLowGain = ParamUtils::createGainParameter ("tls_gain", "Tail LS Gain", -12.0f, 0.0f, -3.0f);
            tailFilterHighCutoff = ParamUtils::createFreqParameter ("ths_cut", "Tail HS Cutoff", 20.0f, 20e3f, 1e3f, 2500.0f);
            tailFilterHighGain = ParamUtils::createGainParameter ("ths_gain", "Tail HS Gain", -12.0f, 0.0f, -3.0f);
            tailModRate = ParamUtils::createFreqParameter ("fdn_mod_rate", "Tail Mod Rate", 0.01f, 5.0f, 0.5f, 0.05f);
            tailModDepth = ParamUtils::createPercentParameter ("fdn_mod_depth", "Tail Mod Depth", 0.2f);
            earlyReflectionGain = ParamUtils::createGainParameter ("er_gain", "Early Reflection Gain", -30.0f, 6.0f, 0.0f);
            diffusionGain = ParamUtils::createGainParameter ("d_gain", "Tail Gain", -30.0f, 6.0f, 0.0f);
            mix = ParamUtils::createPercentParameter ("mix", "Mix", 0.5f);
        }

        ParamUtils::ParamPtr predelay,
                             size,
                             decay,
                             inputFilterLowpassCutoff,
                             inputFilterHighpassCutoff,
                             diffusionModRate,
                             diffusionModDepth,
                             tailFilterLowCutoff,
                             tailFilterLowGain,
                             tailFilterHighCutoff,
                             tailFilterHighGain,
                             tailModRate,
                             tailModDepth,
                             earlyReflectionGain,
                             diffusionGain,
                             mix;
    };

    Reverb (Params& params);
    ~Reverb() override = default;

private:
    void reset() override;
    void process (float* const* buffer, int startIndex, int numSamples) override;

    static constexpr int fdnSize = 8;
    static constexpr int diffusionSteps = 4;
    static constexpr float maxSizeSeconds = 0.2f;
    static constexpr int numLfoPoints = 256;

    Params& parameters;

    juce::SmoothedValue<float> sizeSmoother;
    juce::SmoothedValue<float> decaySmoother;
    juce::SmoothedValue<float> iflpCutoffSmoother;
    juce::SmoothedValue<float> ifhpCutoffSmoother;
    juce::SmoothedValue<float> tlsCutoffSmoother;
    juce::SmoothedValue<float> tlsGainSmoother;
    juce::SmoothedValue<float> thsCutoffSmoother;
    juce::SmoothedValue<float> thsGainSmoother;
    juce::SmoothedValue<float> diffModRateSmoother;
    juce::SmoothedValue<float> diffModDepthSmoother;
    juce::SmoothedValue<float> tailModRateSmoother;
    juce::SmoothedValue<float> tailModDepthSmoother;
    juce::SmoothedValue<float> earlyReflectionGainSmoother;
    juce::SmoothedValue<float> diffusionGainSmoother;
    juce::SmoothedValue<float> predelaySmoother;
    juce::SmoothedValue<float> mixSmoother;

    std::optional<DiffusionReverb<fdnSize, diffusionSteps>> diffusionReverb;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Reverb)
};

} // namespace cgo
