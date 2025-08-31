namespace cgo
{

struct AudioUtils
{
    template <typename T>
    static std::pair<T, T> constantPowerMix (T mix)
    {
        return {
            juce::dsp::FastMathApproximations::cos (juce::MathConstants<T>::halfPi * mix),
            juce::dsp::FastMathApproximations::sin (juce::MathConstants<T>::halfPi * mix)
        };
    }

    template <typename T>
    static void stereoWidth (T& l, T& r, T amount)
    {
        T m = (l + r) * (T) 0.5;
        T s = (l - r) * (T) 0.5;
        s *= amount;

        l = m + s;
        r = m - s;
    }
};

} // namespace cgo
