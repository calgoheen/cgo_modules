namespace cgo
{

class ModulatedValue
{
public:
    /** How often modulation is sampled: once per process call and ramped across it, or every
        sample.
    */
    enum class Rate
    {
        block,
        audio
    };

    const juce::NormalisableRange<float> range;

    /** The smoothing time and rate are fixed for the lifetime of the value. Pass 0 smoothing for
        a value the DSP reads at discrete moments rather than continuously; this does not disable
        modulation. Use `Rate::audio` for parameters that require fast modulation.
    */
    ModulatedValue (const juce::NormalisableRange<float>& range, double smoothingTimeSeconds = 50e-3f, Rate rate = Rate::block);

    void prepare (double sampleRate, int blockSize);

    /** Advances the value by numSamples, summing the modulation from the given slots on top of
        it. Defaults to unmodulated.
    */
    void process (int numSamples, ModulationView view = {});

    /** Sets the value to ramp towards.

        The write is handed to whichever thread processes the value rather than applied here, so
        this is safe to call from any thread at any time.
    */
    void setValue (float newValue);

    /** Same as setValue, but taking a normalized [0, 1] value rather than one in range. */
    void setValueNormalized (float newValue);

    /** Sets the value without smoothing. */
    void snapTo (float newValue);

    /** Same as snapTo, but taking a normalized [0, 1] value rather than one in range. */
    void snapToNormalized (float newValue);

    /** True while the value is ramping towards a new target and for one block after it settles,
        and always true while it is modulated.
    */
    bool isChanging() const;

    /** Forces the next processed block to report isChanging() as true. */
    void markUnsettled();

    /** True when the value was written from outside during the last processed block: a knob, a
        host edit, or automation playing back.
    */
    bool wasTouched() const;

    /** The value per sample across the last processed block. */
    const float* getBuffer() const;

    /** The value at the end of the last processed block, including smoothing and modulation. */
    float getCurrentValue() const;

    /** The value last written by setValue or snapTo, ignoring smoothing and modulation. */
    float getTargetValue() const;

private:
    class PendingValue
    {
    public:
        enum class Kind
        {
            none,
            ramp,
            snap
        };

        struct Write
        {
            Kind kind = Kind::none;
            float value = 0.0f;
        };

        static_assert (sizeof (Write) == sizeof (Kind) + sizeof (float), "Write must have no padding");
        static_assert (std::atomic<Write>::is_always_lock_free);

        void ramp (float newValue);
        void snap (float newValue);

        Write take();
        float peek() const;

    private:
        void store (float newValue, Kind newKind);

        std::atomic<Write> current { Write {} };
    };

    float fromNormalized (float normalized) const;
    float valueAt (float base, ModulationView view, int index) const;
    float writeAudioRate (int numSamples, ModulationView view);
    float writeBlockRate (int numSamples, ModulationView view);

    const double smoothingTimeSeconds;
    const bool smoothed;
    const Rate rate;

    bool dirty { false };
    bool touched { false };
    bool unsettled { false };
    float lastValue { 0.0f };
    std::atomic<float> currentValue { 0.0f };

    PendingValue pending;
    ControlBuffer buffer;
    juce::SmoothedValue<float> smoother;

    JUCE_DECLARE_NON_COPYABLE (ModulatedValue)
};

} // namespace cgo
