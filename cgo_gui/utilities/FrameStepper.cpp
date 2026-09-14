#include <cgo_gui/cgo_gui.h>

namespace cgo
{

FrameStepper::FrameStepper (juce::Component& owner) : attachment (&owner, [this] (double timestampSec) { onVBlank (timestampSec); }) {}

void FrameStepper::add (Listener* listener) { listeners.add (listener); }

void FrameStepper::remove (Listener* listener) { listeners.remove (listener); }

void FrameStepper::setMaxRateHz (double hz)
{
    maxRateHz = juce::jmax (0.0, hz);
    nextStepSec = 0.0; // resynchronise on the next frame
}

double FrameStepper::getMaxRateHz() const { return maxRateHz; }

void FrameStepper::onVBlank (double timestampSec)
{
    const auto previousVBlankSec = std::exchange (lastVBlankSec, timestampSec);
    const auto haveFramePeriod = previousVBlankSec > 0.0 && timestampSec > previousVBlankSec;

    if (maxRateHz > 0.0 && haveFramePeriod)
    {
        const auto framePeriod = timestampSec - previousVBlankSec;
        const auto interval = 1.0 / maxRateHz;

        // Step on whichever frame lands nearest the target time. Without the half
        // frame of slack, a cap that doesn't divide the refresh rate evenly drops
        // a step every few frames instead of pacing evenly.
        if (timestampSec + framePeriod * 0.5 < nextStepSec)
            return;

        nextStepSec += interval;

        // A hidden window or a modal loop can leave the target far in the past.
        // Resynchronise rather than firing a burst of catch-up steps.
        if (nextStepSec < timestampSec)
            nextStepSec = timestampSec + interval;
    }

    listeners.call ([] (Listener& l) { l.step(); });
}

} // namespace cgo
