#include <cgo_graph/cgo_graph.h>

namespace cgo
{

constexpr double resyncToleranceSeconds = 0.001;

void PlayHeadState::updatePlayHead (juce::AudioPlayHead& ph, double blockSeconds)
{
    const auto pos = ph.getPosition();

    if (! pos.hasValue())
        return;

    const double nextPpq = pos->getPpqPosition().orFallback (ppq);
    const bool nextPlaybackState = pos->getIsPlaying();
    const double nextTempo = pos->getBpm().orFallback (tempo);

    const bool expectationHolds = playbackState && nextPlaybackState && juce::exactlyEqual (tempo, nextTempo);
    const double drift = std::abs (nextPpq - expectedPpq) * getSecondsPerBeat();

    ppq = nextPpq;

    if (playbackState != nextPlaybackState)
    {
        playbackState = nextPlaybackState;
        playbackStateChanged();
    }

    if (! juce::exactlyEqual (tempo, nextTempo))
    {
        tempo = nextTempo;
        tempoChanged();
    }

    if (expectationHolds && drift > resyncToleranceSeconds)
        playHeadJumped();

    expectedPpq = ppq + blockSeconds / getSecondsPerBeat();
}

bool PlayHeadState::getPlaybackState() const { return playbackState; }
double PlayHeadState::getPlaybackPosition() const { return ppq; }
double PlayHeadState::getSecondsPerBeat() const { return 60.0 / tempo; }

} // namespace cgo
