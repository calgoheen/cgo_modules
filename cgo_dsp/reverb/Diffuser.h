namespace cgo
{

template <int N, int S>
class Diffuser
{
public:
    using Array = std::array<float, N>;

    Diffuser (int maxDelaySamples)
    {
        for (int i = 0; i < S; i++)
            steps[i].emplace (maxDelaySamples, static_cast<float> (i) / S);
    }

    ~Diffuser() = default;

    void setDelay (float lengthInSamples)
    {
        for (int i = 0; i < S; i++)
            steps[i]->setDelay (lengthInSamples * std::pow (2.0f, (float) -(i + 1)));
    }

    Array process (Array x)
    {
        Array y = x;

        for (auto& step : steps)
            y = step->process (y);

        return y;
    }

    void reset()
    {
        for (auto& step : steps)
            step->reset();
    }

    void setModRate (float rateHz, float sampleRate)
    {
        for (auto& step : steps)
            step->setModRate (rateHz, sampleRate);
    }

    void setModDepth (float depth)
    {
        for (auto& step : steps)
            step->setModDepth (depth);
    }

private:
    std::array<std::optional<DiffusionStep<N>>, S> steps;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Diffuser)
};

} // namespace cgo
