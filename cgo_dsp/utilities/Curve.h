namespace cgo::dsp::Curve
{

template <typename T>
T rationalAlpha (T curve)
{
    jassert (curve > (T) -1 && curve < (T) 1);

    const T c = ((T) 1 + curve) / (T) 2;
    return ((T) 2 * c - (T) 1) / ((T) 1 - c);
}

template <typename T>
T rational (T x, T alpha)
{
    jassert (x >= (T) 0 && x <= (T) 1);

    return ((T) 1 + alpha) * x / ((T) 1 + alpha * x);
}

template <typename T>
T exponential (T x, T curve)
{
    jassert (x >= (T) 0 && x <= (T) 1);
    jassert (curve >= (T) -1 && curve <= (T) 1);

    if (std::abs (curve) < (T) 1e-3)
        return x;

    constexpr T base = (T) 1e6;

    if (curve > (T) 0)
        return (std::pow (base, curve * x) - (T) 1) / (std::pow (base, curve) - (T) 1);
    else
        return (T) 1 - (std::pow (base, -curve * ((T) 1 - x)) - (T) 1) / (std::pow (base, -curve) - (T) 1);
}

} // namespace cgo::dsp::Curve
