namespace cgo
{

template <typename T>
class SmoothingFilter
{
public:
    SmoothingFilter()
    {
        reset (T (0));
        setLength (T (1000));
    }

    ~SmoothingFilter() = default;

    void reset (T initValue)
    {
        y = initValue;
    }

    void setLength (T d)
    {
        a1 = -std::exp (-juce::MathConstants<T>::pi / d);
        b0 = T (1) - std::abs (a1);
    }

    T process (T inputSample)
    {
        y = b0 * inputSample - a1 * y;
        return y;
    }

private:
    T a1, b0, y;

    JUCE_DECLARE_NON_COPYABLE (SmoothingFilter)
};

} // namespace cgo
