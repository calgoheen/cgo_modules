namespace cgo
{

class Reclaimable
{
public:
    Reclaimable() = default;

    virtual ~Reclaimable() = default;
    virtual void reclaim() = 0;

    JUCE_DECLARE_NON_COPYABLE (Reclaimable)
};

class Reclaimer final : private juce::Timer
{
public:
    explicit Reclaimer (int intervalMs = 500) { startTimer (intervalMs); }
    ~Reclaimer() override { stopTimer(); }

    void add (Reclaimable& r)
    {
        JUCE_ASSERT_MESSAGE_THREAD
        jassert (std::find (registered.begin(), registered.end(), &r) == registered.end());
        registered.push_back (&r);
    }

    void remove (Reclaimable& r)
    {
        JUCE_ASSERT_MESSAGE_THREAD

        // A reclaim() can free an object that owns other Reclaimables, so this
        // may run reentrantly from within timerCallback's loop. Tombstone instead of
        // erasing so the in-progress iteration stays valid; compact afterwards.
        if (reclaiming)
            std::replace (registered.begin(), registered.end(), &r, static_cast<Reclaimable*> (nullptr));
        else
            registered.erase (std::remove (registered.begin(), registered.end(), &r), registered.end());
    }

private:
    void timerCallback() override
    {
        {
            juce::ScopedValueSetter<bool> svs (reclaiming, true);

            for (size_t i = 0; i < registered.size(); i++)
                if (auto* r = registered[i])
                    r->reclaim();
        }

        registered.erase (std::remove (registered.begin(), registered.end(), nullptr), registered.end());
    }

    std::vector<Reclaimable*> registered;
    bool reclaiming = false;

    JUCE_DECLARE_NON_COPYABLE (Reclaimer)
};

template <typename T>
class AudioThreadExchange final : public Reclaimable
{
public:
    AudioThreadExchange() { reclaimer.get().add (*this); }
    ~AudioThreadExchange() override { reclaimer.get().remove (*this); }

    void set (std::unique_ptr<T> next)
    {
        const juce::SpinLock::ScopedLockType lock (mutex);
        mainThreadState = std::move (next);
        isNew = true;
    }

    T* loadAudioThreadState()
    {
        const juce::SpinLock::ScopedTryLockType lock (mutex);

        if (lock.isLocked() && isNew)
        {
            std::swap (mainThreadState, audioThreadState);
            isNew = false;
        }

        return audioThreadState.get();
    }

    void reclaim() override
    {
        const juce::SpinLock::ScopedLockType lock (mutex);

        if (! isNew)
            mainThreadState.reset();
    }

private:
    juce::SharedResourcePointer<Reclaimer> reclaimer;
    juce::SpinLock mutex;
    std::unique_ptr<T> mainThreadState, audioThreadState;
    bool isNew = false;

    JUCE_DECLARE_NON_COPYABLE (AudioThreadExchange)
};

} // namespace cgo
