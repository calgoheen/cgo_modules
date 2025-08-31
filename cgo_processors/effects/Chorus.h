namespace cgo
{

class Chorus : public Processor
{
public:
    struct Params
    {
        Params()
        {
            auto widthSfv = [] (float val, int) { return juce::String (juce::roundToInt (val * 100.0f)); };
            auto widthVfs = [] (const juce::String& str) { return str.getFloatValue() / 100.0f; };

            type = ParamUtils::createChoiceParameter ("type", "Type", { "Chorus", "Ensemble" }, 0);
            rate = ParamUtils::createFreqParameter ("rate", "Rate", 0.01f, 20.0f, 2.0f, 1.0f);
            amount = ParamUtils::createPercentParameter ("amount", "Amount", 0.5f);
            feedback = ParamUtils::createPercentParameter ("feedback", "Feedback", 0.0f);
            flipFeedback = ParamUtils::createBoolParameter ("flip_fb", "Flip", false);
            width = ParamUtils::createRangedParameter ("width", "Width", "%", { 0.0f, 2.0f }, 1.0f, std::move (widthSfv), std::move (widthVfs));
            output = ParamUtils::createGainParameter ("output", "Output", -30.0f, 6.0f, 0.0f);
            mix = ParamUtils::createPercentParameter ("mix", "Mix", 0.5f);
        }

        ParamUtils::ParamPtr type, rate, amount, feedback, flipFeedback, width, output, mix;
    };

    Chorus (Params& params);
    ~Chorus() override = default;

private:
    enum class Algorithm
    {
        chorus = 0,
        ensemble
    };

    using StereoSample = std::array<float, 2>;

    void reset() override;
    void process (float* const* buffer, int startIndex, int numSamples) override;
    void playbackStateChanged() override;

    StereoSample processChorus (StereoSample drySample, float amount, float feedback);
    StereoSample processEnsemble (StereoSample drySample, float amount, float feedback);

    static constexpr float delayLengthSeconds = 17e-3f;
    static constexpr int numSplinePoints = 64;

    Params& parameters;

    juce::SmoothedValue<float> rateSmoother;
    juce::SmoothedValue<float> amountSmoother;
    juce::SmoothedValue<float> feedbackSmoother;
    juce::SmoothedValue<float> widthSmoother;
    juce::SmoothedValue<float> outputSmoother;
    juce::SmoothedValue<float> mixSmoother;

    cgo::Phasor phasor;
    float feedbackSign;
    Algorithm algorithm;

    using DelayLine = chowdsp::DelayLine<float, chowdsp::DelayLineInterpolationTypes::Lagrange3rd>;
    std::array<std::optional<DelayLine>, 2> modulatedDelays;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Chorus)
};

} // namespace cgo
