namespace cgo
{

class Phasor
{
public:
    Phasor() = default;

    void inc() 
    { 
        phase = std::fmod (phase + increment, 1.0); 
    }

    double get() const 
    { 
        return phase;
    }
    
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
        const float result = static_cast<float> (phase);
        return result == 1.0f ? 0.0f : result;
    }
    
    float getFloat (float offset) const
    {
        return std::fmod (static_cast<float> (phase) + offset, 1.0f);
    }

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

        increment = freq / sampleRate;
    }

    void setPhase (double p)
    {
        jassert (p >= 0.0);

        phase = std::fmod (p, 1.0);
    }

private:
    double phase { 0.0 };
    double increment { 0.0 };

    JUCE_DECLARE_NON_COPYABLE (Phasor)
};

} // namespace cgo
