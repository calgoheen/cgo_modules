namespace cgo::dsp::reverb::arrayMixers
{

template <typename T, size_t N>
class Hadamard
{
    static_assert (std::is_floating_point_v<T>, "T must be a floating point type");
    static_assert ((N & (N - 1)) == 0 && N > 0, "N must be a power of 2");

public:
    static void mix (std::array<T, N>& data)
    {
        // Fast Walsh-Hadamard Transform
        for (size_t step = 1; step < N; step *= 2)
        {
            for (size_t i = 0; i < N; i += step * 2)
            {
                for (size_t j = i; j < i + step; j++)
                {
                    const auto a = data[j];
                    const auto b = data[j + step];
                    data[j] = a + b;
                    data[j + step] = a - b;
                }
            }
        }

        // Normalize
        const auto scale = (T) (1.0 / std::sqrt ((double) N));
        for (auto& x : data)
            x *= scale;
    }
};

template <typename T, size_t N>
class Householder
{
    static_assert (std::is_floating_point_v<T>, "T must be a floating point type");
    static_assert (N > 0, "N must be greater than 0");

public:
    static void mix (std::array<T, N>& data)
    {
        const auto sum = std::accumulate (data.begin(), data.end(), (T) 0);

        constexpr auto scale = (T) (2.0 / N);
        const auto correction = sum * scale;

        std::for_each (data.begin(), data.end(), [correction] (T& x) { x -= correction; });
    }
};

} // namespace cgo::dsp::reverb::arrayMixers

namespace cgo::dsp::reverb
{

template <int N>
class TapModulator
{
public:
    static constexpr float maxDepth = 0.05f;

    TapModulator (float basePhaseOffset = 0.0f)
    {
        LfoShape::init();

        for (int i = 0; i < N; i++)
            phaseOffsets[(size_t) i] = std::fmod ((float) i / N + basePhaseOffset, 1.0f);
    }

    ~TapModulator() = default;

    float scale (int tap) const
    {
        const float lfo = LfoShape::get (LfoShape::sine, phasor.getFloat (phaseOffsets[(size_t) tap]), (float) phasor.getFrequency());
        const float modVal = 2.0f * lfo - 1.0f;

        return 1.0f + maxDepth * depth * modVal;
    }

    void inc() { phasor.inc(); }

    void reset() { phasor.setPhase (0.0); }

    void setRate (float rateHz, float sampleRate) { phasor.setFrequency (rateHz, sampleRate); }

    void setDepth (float d) { depth = d; }

private:
    std::array<float, N> phaseOffsets;
    Phasor phasor;
    float depth { 0.0f };

    JUCE_DECLARE_NON_COPYABLE (TapModulator)
};

template <int N>
class DiffusionStep
{
public:
    using Array = std::array<float, N>;

    DiffusionStep (int maxDelaySamples, float basePhaseOffset)
      : mod (basePhaseOffset), delay ((int) std::ceil (maxDelaySamples * (1.0f + TapModulator<N>::maxDepth)))
    {
        delay.prepare ({ 0.0, 0, N });

        for (int i = 0; i < N; i++)
        {
            polarity[(size_t) i] = juce::Random::getSystemRandom().nextBool() ? -1.0f : 1.0f;
            spacing[(size_t) i] = juce::Random::getSystemRandom().nextFloat();
        }

        setDelay (0.0f);
    }

    ~DiffusionStep() = default;

    void reset()
    {
        delay.reset();
        mod.reset();
    }

    Array process (Array x)
    {
        Array y;

        for (int i = 0; i < N; i++)
        {
            const float modDelayLength = delayLengthSamples[(size_t) i] * mod.scale (i);

            delay.pushSample (i, x[(size_t) i]);
            y[(size_t) i] = delay.popSample (i, modDelayLength, true);
        }

        arrayMixers::Hadamard<float, N>::mix (y);

        for (int i = 0; i < N; i++)
            y[(size_t) i] *= polarity[(size_t) i];

        mod.inc();

        return y;
    }

    void setDelay (float lengthInSamples)
    {
        const float scale = 1.0f / N;

        for (int i = 0; i < N; i++)
        {
            const float min = lengthInSamples * i * scale;
            const float max = lengthInSamples * (i + 1) * scale;
            delayLengthSamples[(size_t) i] = juce::jmap (spacing[(size_t) i], min, max);
        }
    }

    void setModRate (float rateHz, float sampleRate) { mod.setRate (rateHz, sampleRate); }

    void setModDepth (float depth) { mod.setDepth (depth); }

private:
    Array delayLengthSamples;
    Array polarity;
    Array spacing;

    TapModulator<N> mod;

    using Delay = chowdsp::DelayLine<float, chowdsp::DelayLineInterpolationTypes::Linear>;
    Delay delay;

    JUCE_DECLARE_NON_COPYABLE (DiffusionStep)
};

template <int N, int S>
class Diffuser
{
public:
    using Array = std::array<float, N>;

    Diffuser (int maxDelaySamples)
    {
        for (int i = 0; i < S; i++)
            steps[(size_t) i].emplace (maxDelaySamples, (float) i / S);
    }

    ~Diffuser() = default;

    void setDelay (float lengthInSamples)
    {
        for (int i = 0; i < S; i++)
            steps[(size_t) i]->setDelay (lengthInSamples * std::pow (2.0f, (float) -(i + 1)));
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

    JUCE_DECLARE_NON_COPYABLE (Diffuser)
};

enum class SpacingType
{
    exponential = 0,
    logarithmic,
    linear
};

enum class DecayFilterType
{
    lowShelf = 0,
    highShelf,
    numTypes
};

template <int N, SpacingType Spacing>
inline void computeTapLengths (std::array<float, N>& lengths, float baseLengthSamples)
{
    const float d = juce::jmax (baseLengthSamples, 2.0f);

    for (int i = 0; i < N; i++)
    {
        const float x = (float) i / N;

        const float y = [x]
        {
            if constexpr (Spacing == SpacingType::exponential)
                return std::pow (2.0f, x) - 1.0f;
            else if constexpr (Spacing == SpacingType::logarithmic)
                return std::log2 (x + 1.0f);
            else
                return x;
        }();

        lengths[(size_t) i] = d * (1.0f + y);
    }
}

template <int N, SpacingType Spacing = SpacingType::exponential>
class Tail
{
public:
    using Array = std::array<float, N>;

    Tail (int maxDelaySamples) : delay ((int) std::ceil (2 * maxDelaySamples * (1.0f + TapModulator<N>::maxDepth)))
    {
        delay.prepare ({ 0.0, 0, (juce::uint32) N });

        for (int i = 0; i < (int) DecayFilterType::numTypes; i++)
            filter[(size_t) i].prepare (N);

        setFeedback (0.0f);
        setFilterCoefs (DecayFilterType::lowShelf, 0.0f, 250.0f, 48e3f);
        setFilterCoefs (DecayFilterType::highShelf, 0.0f, 2500.0f, 48e3f);

        setDelay (0.0f);
    }

    ~Tail() = default;

    Array process (Array x)
    {
        Array y;

        for (int i = 0; i < N; i++)
            y[(size_t) i] = delay.popSample (i, delayLengthSamples[(size_t) i] * mod.scale (i), true);

        arrayMixers::Householder<float, N>::mix (y);

        for (int i = 0; i < N; i++)
        {
            for (size_t j = 0; j < (size_t) DecayFilterType::numTypes; ++j)
                y[(size_t) i] = filter[j].processSample (y[(size_t) i], i);

            delay.pushSample (i, x[(size_t) i] + feedback * y[(size_t) i]);
        }

        mod.inc();

        return y;
    }

    void reset()
    {
        delay.reset();

        for (size_t i = 0; i < (size_t) DecayFilterType::numTypes; i++)
            filter[i].reset();

        mod.reset();
    }

    void setDelay (float lengthInSamples) { computeTapLengths<N, Spacing> (delayLengthSamples, lengthInSamples); }

    void setFeedback (float f) { feedback = f; }

    void setFilterCoefs (DecayFilterType filterType, float gainDb, float fc, float fs)
    {
        const float gain = juce::Decibels::decibelsToGain (gainDb);
        const bool isLow = filterType == DecayFilterType::lowShelf;

        filter[(size_t) filterType].calcCoefs (isLow ? gain : 1.0f, isLow ? 1.0f : gain, fc, fs);
    }

    void setModRate (float rateHz, float sampleRate) { mod.setRate (rateHz, sampleRate); }

    void setModDepth (float depth) { mod.setDepth (depth); }

private:
    Array delayLengthSamples;
    float feedback { 0.0f };

    TapModulator<N> mod;

    using Delay = chowdsp::DelayLine<float, chowdsp::DelayLineInterpolationTypes::Linear>;
    Delay delay;

    std::array<chowdsp::ShelfFilter<float>, (size_t) DecayFilterType::numTypes> filter;

    JUCE_DECLARE_NON_COPYABLE (Tail)
};

template <int N, SpacingType Spacing = SpacingType::exponential>
class EarlyReflections
{
public:
    using Array = std::array<float, N>;

    EarlyReflections (int maxDelaySamples) : delay (2 * maxDelaySamples)
    {
        delay.prepare ({ 0.0, 0, (juce::uint32) N });
        setDelay (0.0f);
    }

    ~EarlyReflections() = default;

    Array process (Array x)
    {
        Array y;

        for (int i = 0; i < N; i++)
            y[(size_t) i] = delay.popSample (i, delayLengthSamples[(size_t) i], true);

        arrayMixers::Householder<float, N>::mix (y);

        for (int i = 0; i < N; i++)
            delay.pushSample (i, x[(size_t) i]);

        return y;
    }

    void reset() { delay.reset(); }

    void setDelay (float lengthInSamples) { computeTapLengths<N, Spacing> (delayLengthSamples, lengthInSamples); }

private:
    Array delayLengthSamples;

    using Delay = chowdsp::DelayLine<float, chowdsp::DelayLineInterpolationTypes::Linear>;
    Delay delay;

    JUCE_DECLARE_NON_COPYABLE (EarlyReflections)
};

} // namespace cgo::dsp::reverb

namespace cgo::dsp
{

using DiffusionReverbDecayFilterType = reverb::DecayFilterType;

template <int N, int S>
class DiffusionReverb
{
public:
    using Array = std::array<float, N>;

    DiffusionReverb (float sr, int numChans, float maxSizeSeconds)
      : sampleRate (sr),
        numChannels (numChans),
        maxRoomSizeSamples ((int) std::ceil (maxSizeSeconds * sampleRate) + 1),
        maxPredelaySamples ((int) std::ceil (maxPredelaySeconds * sampleRate) + 1),
        fdn (maxRoomSizeSamples),
        earlyReflections (maxRoomSizeSamples / 2 + 1),
        diffuser (maxRoomSizeSamples),
        predelay (maxPredelaySamples)
    {
        jassert (numChannels == 2);

        predelay.prepare ({ 0.0, 0, (juce::uint32) numChannels });
        predelay.reset();

        inputLowpass.prepare (numChannels);
        inputLowpass.reset();

        inputHighpass.prepare (numChannels);
        inputHighpass.reset();

        setSize (0.1f);
        setDecay (3.0f);
        setInputFilter (1e3f, 10.0f);
        setDecayFilter (DiffusionReverbDecayFilterType::lowShelf, 0.0f, 250.0f);
        setDecayFilter (DiffusionReverbDecayFilterType::highShelf, 0.0f, 2500.0f);
        setEarlyReflectionGain (1.0f);
        setDiffusionGain (1.0f);
        setPredelay (0.0005f);
        setMix (1.0f);
    }

    ~DiffusionReverb() = default;

    void process (float* const* buffer, int startSample, int numSamples)
    {
        for (int n = 0; n < numSamples; n++)
        {
            const int index = startSample + n;

            const float dryL = buffer[0][index];
            const float dryR = buffer[1][index];

            predelay.pushSample (0, buffer[0][index]);
            predelay.pushSample (1, buffer[1][index]);

            float l = predelay.popSample (0, predelaySamples, true);
            float r = predelay.popSample (1, predelaySamples, true);

            l = inputLowpass.processSample (l, 0);
            l = inputHighpass.processSample (l, 0);
            r = inputLowpass.processSample (r, 1);
            r = inputHighpass.processSample (r, 1);

            Array y;
            for (int i = 0; i < N; i += 2)
            {
                y[(size_t) i] = l;
                y[(size_t) i + 1] = r;
            }

            const auto diffuserOut = diffuser.process (y);
            const auto erOut = earlyReflections.process (diffuserOut);
            y = fdn.process (diffuserOut);

            for (int i = 0; i < N; i++)
                y[(size_t) i] = y[(size_t) i] * diffusionGain + erOut[(size_t) i] * earlyReflectionGain;

            l = 0.0f;
            r = 0.0f;
            for (int i = 0; i < N; i += 2)
            {
                l += y[(size_t) i];
                r += y[(size_t) i + 1];
            }

            const float scale = 4.0f * wetGain / (float) N;

            buffer[0][index] = dryL * dryGain + l * scale;
            buffer[1][index] = dryR * dryGain + r * scale;
        }
    }

    void reset()
    {
        fdn.reset();
        diffuser.reset();
        earlyReflections.reset();
        predelay.reset();

        inputLowpass.reset();
        inputHighpass.reset();
    }

    void setSize (float lengthInSeconds)
    {
        jassert (std::ceil (lengthInSeconds * sampleRate) < maxRoomSizeSamples);

        const float roomSizeSamples = lengthInSeconds * sampleRate;
        fdn.setDelay (roomSizeSamples);
        earlyReflections.setDelay (0.5f * roomSizeSamples);
        diffuser.setDelay (0.5f * roomSizeSamples);

        roomSizeSeconds = lengthInSeconds;
        updateDecayLength();
    }

    void setDecay (float lengthInSeconds)
    {
        decayLengthSeconds = lengthInSeconds;
        updateDecayLength();
    }

    void setInputFilter (float centerFrequencyHz, float widthInOctaves)
    {
        const float center = juce::jlimit (minCutoffHz, maxCutoffRatio * sampleRate, centerFrequencyHz);
        const float halfSpan = std::pow (2.0f, 0.5f * juce::jmax (minBandwidthOctaves, widthInOctaves));

        inputLowpass.calcCoefs (juce::jmin (center * halfSpan, maxCutoffRatio * sampleRate), inputFilterQ, sampleRate);
        inputHighpass.calcCoefs (juce::jmax (center / halfSpan, minCutoffHz), inputFilterQ, sampleRate);
    }

    void setDecayFilter (DiffusionReverbDecayFilterType filterType, float gainInDb, float cutoffHz)
    {
        fdn.setFilterCoefs (filterType, gainInDb, cutoffHz, sampleRate);
    }

    void setEarlyReflectionGain (float linearGain) { earlyReflectionGain = linearGain; }

    void setDiffusionGain (float linearGain) { diffusionGain = linearGain; }

    void setTailModRate (float rateHz) { fdn.setModRate (rateHz, sampleRate); }

    void setTailModDepth (float depth) { fdn.setModDepth (depth); }

    void setDiffusionModRate (float rateHz) { diffuser.setModRate (rateHz, sampleRate); }

    void setDiffusionModDepth (float depth) { diffuser.setModDepth (depth); }

    void setPredelay (float lengthInSeconds) { predelaySamples = lengthInSeconds * sampleRate; }

    void setMix (float amount)
    {
        const auto [dry, wet] = AudioUtils::constantPowerMix (amount);

        dryGain = dry;
        wetGain = wet;
    }

private:
    void updateDecayLength()
    {
        const float numLoops = decayLengthSeconds / roomSizeSeconds;
        const float gainPerLoop = -60.0f / numLoops;

        fdn.setFeedback (std::pow (10.0f, gainPerLoop * 0.05f));
    }

    static constexpr float maxPredelaySeconds = 0.25f;
    static constexpr float inputFilterQ = 0.71f;
    static constexpr float minCutoffHz = 10.0f;
    static constexpr float maxCutoffRatio = 0.49f;
    static constexpr float minBandwidthOctaves = 0.5f;

    const float sampleRate;
    const int numChannels;
    const int maxRoomSizeSamples;
    const int maxPredelaySamples;

    float roomSizeSeconds;
    float decayLengthSeconds;
    float earlyReflectionGain;
    float diffusionGain;
    float predelaySamples;
    float dryGain;
    float wetGain;

    reverb::Tail<N> fdn;
    reverb::EarlyReflections<N, reverb::SpacingType::linear> earlyReflections;
    reverb::Diffuser<N, S> diffuser;

    using PredelayLine = chowdsp::DelayLine<float, chowdsp::DelayLineInterpolationTypes::Linear>;
    PredelayLine predelay;

    chowdsp::SecondOrderLPF<float> inputLowpass;
    chowdsp::SecondOrderHPF<float> inputHighpass;

    JUCE_DECLARE_NON_COPYABLE (DiffusionReverb)
};

} // namespace cgo::dsp
