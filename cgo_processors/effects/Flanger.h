namespace cgo
{

class Flanger : public Processor
{
public:
    struct Params
    {
        Params()
        {
            delay = ParamUtils::createTimeParameter ("delay", "Delay Time", minDelaySeconds, maxDelaySeconds, 2e-3f, 2.5e-3f);
            feedback = ParamUtils::createPercentParameter ("feedback", "Feedback", 0.0f);
            flipFeedback = ParamUtils::createBoolParameter ("flip_fb", "Flip", false);
            shape = ParamUtils::createChoiceParameter ("shape", "Shape", { "Triangle", "Sine", "Ramp Up", "Ramp Down" }, 0);
            sync = ParamUtils::createBoolParameter ("sync", "Sync", true);
            rateFree = ParamUtils::createFreqParameter ("rate_free", "Rate Free", 0.0f, 10.0f, 1.0f, 1.0f);
            rateSync = ParamUtils::createSyncedRateParameter ("rate_sync", "Rate Sync");
            depth = ParamUtils::createPercentParameter ("depth", "Depth", 1.0f);
            offset = ParamUtils::createPercentParameter ("offset", "Offset", 0.5f);
            safeBass = ParamUtils::createFreqParameter ("safe_bass", "Safe Bass", 10.0f, 3000.0f, 500.0f, 100.0f);
            warmth = ParamUtils::createPercentParameter ("warmth", "Warmth", 0.0f);
            output = ParamUtils::createGainParameter ("output", "Output", -30.0f, 6.0f, 0.0f);
            mix = ParamUtils::createPercentParameter ("mix", "Mix", 1.0f);
        }

        ParamUtils::ParamPtr delay, feedback, flipFeedback, shape, sync, rateFree, rateSync, depth, offset, safeBass, warmth, output, mix;
    };

    Flanger (Params& params);
    ~Flanger() override = default;

private:
    void reset() override;
    void process (float* const* buffer, int startIndex, int numSamples) override;
    void playbackStateChanged() override;
    void tempoChanged() override;
    void updateRateSync();

    static constexpr double minDelaySeconds { 0.1e-3 };
    static constexpr double maxDelaySeconds { 20e-3 };
    static constexpr int numSplinePoints = 64;

    Params& parameters;

    juce::SmoothedValue<float> delaySmoother;
    juce::SmoothedValue<float> feedbackSmoother;
    juce::SmoothedValue<float> rateSmoother;
    juce::SmoothedValue<float> depthSmoother;
    juce::SmoothedValue<float> offsetSmoother;
    juce::SmoothedValue<float> highpassSmoother;
    juce::SmoothedValue<float> lowpassSmoother;
    juce::SmoothedValue<float> outputSmoother;
    juce::SmoothedValue<float> mixSmoother;

    cgo::Phasor phasor;
    LfoTable::Shape shape;
    float feedbackSign;
    bool rateSyncActive;

    using DelayLine = chowdsp::DelayLine<float, chowdsp::DelayLineInterpolationTypes::Lagrange3rd>;
    std::optional<DelayLine> modulatedDelay;

    using HighpassFilter = chowdsp::SecondOrderHPF<float, chowdsp::CoefficientCalculators::CoefficientCalculationMode::Decramped>;
    HighpassFilter highpass;

    using LowpassFilter = chowdsp::FirstOrderLPF<float>;
    LowpassFilter lowpass;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Flanger)
};

} // namespace cgo
