namespace cgo
{

class PlayHeadState
{
public:
    PlayHeadState() = default;

    virtual ~PlayHeadState() = default;

protected:
    virtual void playbackStateChanged() {}
    virtual void tempoChanged() {}
    virtual void playHeadJumped() {}

    void updatePlayHead (juce::AudioPlayHead&, double blockSeconds);

    bool getPlaybackState() const;
    double getPlaybackPosition() const;
    double getSecondsPerBeat() const;

private:
    bool playbackState { false };
    double ppq { 0.0 };
    double expectedPpq { 0.0 };
    double tempo { 120.0 };

    JUCE_DECLARE_NON_COPYABLE (PlayHeadState)
};

} // namespace cgo
