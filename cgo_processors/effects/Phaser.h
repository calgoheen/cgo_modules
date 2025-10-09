namespace cgo
{

class Phaser : public Processor
{
public:
    struct Params
    {
        Params()
        {
            notches = ParamUtils::createRangedParameter ("notches", "Notches", "", { 1.0f, static_cast<float> (maxNotches), 1.0f }, 4.0f);
            center = ParamUtils::createFreqParameter ("center", "Center", 70.0f, 18.5e3f, 1e3f, 1e3f);
            spread = ParamUtils::createPercentParameter ("spread", "Spread", 0.5f);
            feedback = ParamUtils::createPercentParameter ("feedback", "Feedback", 0.0f);
            flipFeedback = ParamUtils::createBoolParameter ("flip_fb", "Flip", false);
            shape = ParamUtils::createChoiceParameter ("shape", "Shape", { "Triangle", "Sine", "Ramp Up", "Ramp Down" }, 0);
            sync = ParamUtils::createBoolParameter ("sync", "Sync", false);
            rateFree = ParamUtils::createFreqParameter ("rate_free", "Rate Free", 0.0f, 10.0f, 1.0f, 1.0f);
            rateSync = ParamUtils::createSyncedRateParameter ("rate_sync", "Rate Sync");
            amount = ParamUtils::createPercentParameter ("amount", "Amount", 1.0f);
            blend = ParamUtils::createPercentParameter ("blend", "Blend", 0.0f);
            offset = ParamUtils::createPercentParameter ("offset", "Offset", 0.5f);
            safeBass = ParamUtils::createFreqParameter ("safe_bass", "Safe Bass", 10.0f, 3000.0f, 500.0f, 10.0f);
            warmth = ParamUtils::createPercentParameter ("warmth", "Warmth", 0.0f);
            output = ParamUtils::createGainParameter ("output", "Output", -30.0f, 6.0f, 0.0f);
            mix = ParamUtils::createPercentParameter ("mix", "Mix", 0.5f);
        }

        ParamUtils::ParamPtr notches, center, spread, feedback, flipFeedback, shape, sync, rateFree, rateSync, amount, blend, offset, safeBass, warmth, output, mix;
    };

    Phaser (Params& params);
    ~Phaser() override = default;

private:
    void reset() override;
    void process (float* const* buffer, int startIndex, int numSamples) override;
    void playbackStateChanged() override;
    void tempoChanged() override;
    void updateRateSync();

    static constexpr int maxNotches = 42;
    static constexpr int maxNumChannels = 2;
    static constexpr int numSplinePoints = 64;

    Params& parameters;

    juce::SmoothedValue<float> centerSmoother;
    juce::SmoothedValue<float> spreadSmoother;
    juce::SmoothedValue<float> feedbackSmoother;
    juce::SmoothedValue<float> rateSmoother;
    juce::SmoothedValue<float> amountSmoother;
    juce::SmoothedValue<float> blendSmoother;
    juce::SmoothedValue<float> offsetSmoother;
    juce::SmoothedValue<float> highpassSmoother;
    juce::SmoothedValue<float> lowpassSmoother;
    juce::SmoothedValue<float> outputSmoother;
    juce::SmoothedValue<float> mixSmoother;

    int notches;
    LfoTable::Shape shape;
    float feedbackSign;
    bool rateSyncActive;
    std::vector<float> prevFilterOut;

    cgo::Phasor phasor;

    using HighpassFilter = chowdsp::SecondOrderHPF<float, chowdsp::CoefficientCalculators::CoefficientCalculationMode::Decramped>;
    HighpassFilter highpass;

    using LowpassFilter = chowdsp::FirstOrderLPF<float>;
    LowpassFilter lowpass;

    using AllpassFilter = std::array<chowdsp::SVFAllpass<float>, maxNumChannels>;
    AllpassFilter allpassFilters;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Phaser)
};

} // namespace cgo
