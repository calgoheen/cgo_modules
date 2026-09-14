namespace cgo
{

class FrameStepper
{
public:
    struct Listener
    {
        virtual ~Listener() = default;
        virtual void step() = 0;
    };

    explicit FrameStepper (juce::Component& owner);

    void add (Listener* listener);
    void remove (Listener* listener);

    void setMaxRateHz (double hz);
    double getMaxRateHz() const;

private:
    void onVBlank (double timestampSec);

    juce::ListenerList<Listener> listeners;

    double maxRateHz = 0.0;
    double lastVBlankSec = 0.0;
    double nextStepSec = 0.0;

    juce::VBlankAttachment attachment;

    JUCE_DECLARE_NON_COPYABLE (FrameStepper)
};

} // namespace cgo
