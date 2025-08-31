namespace cgo
{

class ProcessParameterUpdater
{
public:
    ProcessParameterUpdater() = default;
    ~ProcessParameterUpdater() = default;

    void add (juce::RangedAudioParameter& param, std::function<void (float)> onChange);
    void update();
    void flush();

private:
    class Updater
    {
    public:
        Updater (juce::RangedAudioParameter& p, std::function<void (float)> f)
            : parameter (p),
              value (0.0f),
              dirty (false),
              listener (p, [this] (float x)
                        {
                            value = x;
                            dirty = true;
                        }),
              callback (std::move (f))
        {
        }

        juce::RangedAudioParameter& parameter;

        std::atomic<float> value;
        std::atomic<bool> dirty;
        std::function<void (float)> callback;

    private:
        ParameterListener listener;

        JUCE_DECLARE_NON_COPYABLE (Updater)
    };

    juce::OwnedArray<Updater> updaters;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ProcessParameterUpdater)
};

} // namespace cgo
