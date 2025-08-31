namespace cgo
{

class Processor
{
public:
    void prepareToPlay (double sampleRate, int samplesPerBlock, int numChannels);
    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi, juce::AudioPlayHead* playHead = nullptr);

    double getLatencySamples() const;

    static constexpr double smoothingTimeSeconds { 75e-3 };

protected:
    virtual ~Processor() = default;

    virtual void playbackStateChanged() {}
    virtual void tempoChanged() {}

    virtual void reset() = 0;
    virtual void process (float* const* buffer, int startIndex, int numSamples) = 0;

    template <typename... Args>
    void addParameterUpdaters (juce::RangedAudioParameter& param, std::function<void (float)>&& updateFunc, Args&&... args)
    {
        parameterUpdater.add (param, std::move (updateFunc));
        addParameterUpdaters (std::forward<Args> (args)...);
    }

    juce::dsp::ProcessSpec getProcessSpec() const;
    double getSampleRate() const;
    int getBlockSize() const;
    int getNumChannels() const;
    bool getPlaybackState() const;
    double getPlaybackPosition() const;
    double getSecondsPerBeat() const;

    void setLatencySamples (double latency);

private:
    void addParameterUpdaters() {}
    void updatePlayHead (juce::AudioPlayHead& ph);

    ProcessParameterUpdater parameterUpdater;

    double currentSampleRate { 0.0 };
    int currentBlockSize { 0 };
    int currentNumChannels { 0 };
    double latencySamples { 0.0 };

    bool playbackState { false };
    double ppq { 0.0 };
    double tempo { 120.0 };
};

} // namespace cgo
