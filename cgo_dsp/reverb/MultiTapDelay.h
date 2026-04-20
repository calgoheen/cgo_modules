namespace cgo
{

namespace MultiTapDelay
{
 
enum class SpacingType
{
    exponential = 0,
    logarithmic,
    linear
};
 
enum class FilterType
{
    lowShelf = 0,
    highShelf,
    numTypes
};
 
namespace detail
{

template <int N, SpacingType Spacing>
inline void computeTapLengths (std::array<float, N>& lengths, float baseLengthSamples)
{
    const float d = juce::jmax (baseLengthSamples, 2.0f);

    for (int i = 0; i < N; ++i)
    {
        const float x = static_cast<float> (i) / N;

        const float y = [x]
        {
            if constexpr (Spacing == SpacingType::exponential)
                return std::pow (2.0f, x) - 1.0f;
            else if constexpr (Spacing == SpacingType::logarithmic)
                return std::log2 (x + 1.0f);
            else
                return x;
        }();

        lengths[i] = d * (1.0f + y);
    }
}

} // namespace detail

template <int N, SpacingType Spacing = SpacingType::exponential>
class Tail
{
public:
    using Array = std::array<float, N>;
 
    Tail (int maxDelaySamples)
        : delay (static_cast<int> (std::ceil (2 * maxDelaySamples * (1.0f + maxModDepth))))
    {
        delay.prepare ({ 0.0, 0, (juce::uint32) N });
 
        for (int i = 0; i < (int) FilterType::numTypes; ++i)
            filter[i].prepare (N);
 
        for (int i = 0; i < N; ++i)
            phaseOffsets[i] = static_cast<float> (i) / N;
 
        setFeedback (0.0f);
        setFilterCoefs (FilterType::lowShelf, 0.0f, 250.0f, 48e3f);
        setFilterCoefs (FilterType::highShelf, 0.0f, 2500.0f, 48e3f);
 
        setDelay (0.0f);
    }
 
    ~Tail() = default;
 
    Array process (Array x)
    {
        Array y;
 
        for (int i = 0; i < N; ++i)
        {
            const float lfo = LfoTable::getLinear<numLfoPoints> (LfoTable::sine, phasor.getFloat (phaseOffsets[i]));
            const float modVal = 2.0f * lfo - 1.0f;
            const float modDelayLength = delayLengthSamples[i] * (1.0f + maxModDepth * modDepth * modVal);
            y[i] = delay.popSample (i, modDelayLength, true);
        }
 
        ArrayMixers::Householder<float, N>::mix (y);
 
        for (int i = 0; i < N; ++i)
        {
            for (size_t j = 0; j < (size_t) FilterType::numTypes; ++j)
                y[i] = filter[j].processSample (y[i], i);
 
            delay.pushSample (i, x[i] + feedback * y[i]);
        }
 
        phasor.inc();
 
        return y;
    }
 
    void reset()
    {
        delay.reset();
 
        for (size_t i = 0; i < (size_t) FilterType::numTypes; ++i)
            filter[i].reset();
 
        phasor.setPhase (0.0);
    }
 
    void setDelay (float lengthInSamples)
    {
        detail::computeTapLengths<N, Spacing> (delayLengthSamples, lengthInSamples);
    }
 
    void setFeedback (float f)
    {
        feedback = f;
    }
 
    void setFilterCoefs (FilterType filterType, float gainDb, float fc, float fs)
    {
        const auto index = (size_t) filterType;
        const auto gainDbPair = filterType == FilterType::lowShelf ? std::pair<float, float> (gainDb, 0.0f)
                                                                   : std::pair<float, float> (0.0f, gainDb);
 
        filter[index].calcCoefs (juce::Decibels::decibelsToGain (gainDbPair.first),
                                 juce::Decibels::decibelsToGain (gainDbPair.second),
                                 fc,
                                 fs);
    }
 
    void setModRate (float rateHz, float sampleRate)
    {
        phasor.setFrequency (rateHz, sampleRate);
    }
 
    void setModDepth (float depth)
    {
        modDepth = depth;
    }
 
private:
    static constexpr int numLfoPoints = 256;
    static constexpr float maxModDepth = 0.05f;
 
    Array delayLengthSamples;
    Array phaseOffsets;
    float feedback { 0.0f };
 
    using Delay = chowdsp::DelayLine<float, chowdsp::DelayLineInterpolationTypes::Linear>;
    Delay delay;
 
    std::array<chowdsp::ShelfFilter<float>, 2> filter;
 
    Phasor phasor;
    float modDepth { 0.0f };
 
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Tail)
};

template <int N, SpacingType Spacing = SpacingType::exponential>
class EarlyReflections
{
public:
    using Array = std::array<float, N>;
 
    EarlyReflections (int maxDelaySamples)
        : delay (2 * maxDelaySamples)
    {
        delay.prepare ({ 0.0, 0, (juce::uint32) N });
        setDelay (0.0f);
    }
 
    ~EarlyReflections() = default;
 
    Array process (Array x)
    {
        Array y;
 
        for (int i = 0; i < N; ++i)
            y[i] = delay.popSample (i, delayLengthSamples[i], true);
 
        ArrayMixers::Householder<float, N>::mix (y);
 
        for (int i = 0; i < N; ++i)
            delay.pushSample (i, x[i]);
 
        return y;
    }
 
    void reset()
    {
        delay.reset();
    }
 
    void setDelay (float lengthInSamples)
    {
        detail::computeTapLengths<N, Spacing> (delayLengthSamples, lengthInSamples);
    }
 
private:
    Array delayLengthSamples;
 
    using Delay = chowdsp::DelayLine<float, chowdsp::DelayLineInterpolationTypes::Linear>;
    Delay delay;
 
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EarlyReflections)
};
 
} // namespace MultiTapDelay

} // namespace cgo
