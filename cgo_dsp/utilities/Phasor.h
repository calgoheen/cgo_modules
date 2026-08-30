namespace cgo::dsp
{

class Phasor
{
public:
    Phasor() = default;

    void inc() { phase = std::fmod (phase + increment, 1.0); }

    double get() const { return phase; }

    double get (double offset) const
    {
        jassert (offset >= 0.0);

        return std::fmod (phase + offset, 1.0);
    }

    double getAndInc()
    {
        const double out = get();
        inc();
        return out;
    }

    float getFloat() const
    {
        const float result = (float) phase;
        return result == 1.0f ? 0.0f : result;
    }

    float getFloat (float offset) const { return std::fmod ((float) phase + offset, 1.0f); }

    float getFloatAndInc()
    {
        const float out = getFloat();
        inc();
        return out;
    }

    void setFrequency (double freq, double sampleRate)
    {
        jassert (freq >= 0.0);
        jassert (sampleRate >= 0.0);

        frequency = freq;
        increment = freq / sampleRate;
    }

    double getFrequency() const { return frequency; }

    void setPhase (double p)
    {
        jassert (p >= 0.0);

        phase = std::fmod (p, 1.0);
    }

private:
    double phase { 0.0 };
    double increment { 0.0 };
    double frequency { 0.0 };
};

} // namespace cgo::dsp
