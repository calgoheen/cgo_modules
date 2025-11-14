namespace cgo
{

class TapeStop : public Processor
{
public:
    struct Params
    {
        Params()
        {
            mode = ParamUtils::createChoiceParameter ("mode", "Mode", { "Bypass", "Stop", "Start" }, 0);
            autoBypass = ParamUtils::createBoolParameter ("auto_byp", "Auto Bypass", false);
            sync = ParamUtils::createBoolParameter ("sync", "Sync", true);
            slowdownLength = ParamUtils::createTimeParameter ("slw_len", "Slowdown Length", 0.01f, 10.0f, 1.0f, 1.0f);
            slowdownLengthSync = ParamUtils::createSyncedRateParameter ("slw_len_s", "Slowdown Length Sync");
            slowdownCurve = ParamUtils::createRangedParameter ("slw_cur", "Slowdown Curve", "", { -1.0f, 1.0f }, 0.0f);
            slowdownStart = ParamUtils::createPercentParameter ("slw_strt", "Slowdown Start", 0.0f);
            slowdownEnd = ParamUtils::createPercentParameter ("slw_end", "Slowdown End", 1.0f);
            speedupLength = ParamUtils::createTimeParameter ("spd_len", "Speedup Length", 0.01f, 10.0f, 1.0f, 1.0f);
            speedupLengthSync = ParamUtils::createSyncedRateParameter ("spd_len_s", "Speedup Length Sync");
            speedupCurve = ParamUtils::createRangedParameter ("spd_cur", "Speedup Curve", "", { -1.0f, 1.0f }, 0.0f);
            speedupStart = ParamUtils::createPercentParameter ("spd_strt", "Speedup Start", 0.0f);
            speedupEnd = ParamUtils::createPercentParameter ("spd_end", "Speedup End", 1.0f);
            fadeLength = ParamUtils::createPercentParameter ("fade_len", "Fade", 0.1f);
            crossfadeLength = ParamUtils::createTimeParameter ("xfade_len", "Crossfade", 0.005f, 0.5f, 0.1f, 0.01f);
            filterType = ParamUtils::createChoiceParameter ("flt_typ", "Filter Type", { "LP", "HP", "BP" }, 1);
            filterCutoff = ParamUtils::createFreqParameter ("flt_cut", "Filter Cutoff", 10.0f, 22e3f, 2e3f, 100.0f);
            filterResonance = ParamUtils::createRangedParameter ("flt_res", "Filter Resonance", "", ParamUtils::getRangeWithCenter (0.1f, 5.0f, 1.0f), 0.71f);
        }

        ParamUtils::ParamPtr mode,
                             autoBypass, 
                             sync, 
                             slowdownLength, 
                             slowdownLengthSync, 
                             slowdownCurve, 
                             slowdownStart, 
                             slowdownEnd, 
                             speedupLength, 
                             speedupLengthSync, 
                             speedupCurve, 
                             speedupStart, 
                             speedupEnd, 
                             fadeLength, 
                             crossfadeLength,
                             filterType,
                             filterCutoff,
                             filterResonance;
    };

    TapeStop (Params& parameters);
    ~TapeStop() override = default;

private:
    using DelayLine = chowdsp::DelayLine<float, chowdsp::DelayLineInterpolationTypes::Lagrange5th>;
    using Filter = chowdsp::StateVariableFilter<float, chowdsp::StateVariableFilterType::MultiMode>;

    enum class Mode
    {
        bypass = 0,
        stop,
        start
    };

    struct Settings
    {
        Mode mode;

        int counter;
        int length;
        int fadeLength;
        int crossfadeLength;

        double delay;
        double curve;
        double start;
        double end;

        Filter* filter;
    };

    void reset() override;
    void process (float* const* buffer, int startIndex, int numSamples) override;
    void tempoChanged() override;

    void processSample (float* const* buffer, int sampleIndex, Settings& settings);
    void updateMode (Mode nextMode);
    void updateLengths();

    static constexpr float maxDelaySeconds = 20.0f;

    Params& parameters;

    juce::SmoothedValue<float> cutoffSmoother;
    juce::SmoothedValue<float> resonanceSmoother;

    bool syncActive;
    bool autoBypass;

    int slowdownLengthSamples, speedupLengthSamples;
    double slowdownCurve, speedupCurve;
    double slowdownStart, speedupStart;
    double slowdownEnd, speedupEnd;
    float fadeLengthProportion;
    int crossfadeLengthSamples;

    Settings currentSettings, prevSettings;

    juce::AudioBuffer<float> crossfadeBuffer;

    std::optional<DelayLine> delay;
    std::array<Filter, 2> filters;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TapeStop)
};

} // namespace cgo
