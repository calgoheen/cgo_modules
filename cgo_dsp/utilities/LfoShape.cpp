#include <cgo_dsp/cgo_dsp.h>

namespace cgo::dsp::LfoShape
{

CGO_ANON_NAMESPACE_BEGIN

constexpr size_t numSinePoints = 512;
constexpr float fallSeconds = 0.02f;
constexpr float minFall = 0.002f;
constexpr float maxFall = 0.25f;

const juce::dsp::LookupTableTransform<float>& sineTable()
{
    static const juce::dsp::LookupTableTransform<float> table { [] (float x) { return (std::sin (juce::MathConstants<float>::twoPi * x) + 1.0f) / 2.0f; },
                                                                0.0f,
                                                                1.0f,
                                                                numSinePoints };

    return table;
}

float ramp (float phase, float freqHz)
{
    const float fall = juce::jlimit (minFall, maxFall, fallSeconds * freqHz);
    const float peak = 1.0f - fall;

    if (phase < peak)
        return phase / peak;

    const float t = (phase - peak) / fall;
    return 1.0f - t * t * (3.0f - 2.0f * t);
}

float square (float phase, float freqHz)
{
    const float edge = juce::jlimit (minFall, maxFall, fallSeconds * freqHz);
    const float holdEnd = 0.5f - edge;

    const bool high = phase < 0.5f;
    const float local = high ? phase : phase - 0.5f;

    if (local < holdEnd)
        return high ? 1.0f : 0.0f;

    const float t = (local - holdEnd) / edge;
    const float s = t * t * (3.0f - 2.0f * t);

    return high ? 1.0f - s : s;
}

CGO_ANON_NAMESPACE_END

void init() { juce::ignoreUnused (ANON::sineTable()); }

float get (Shape shape, float phase, float freqHz)
{
    jassert (phase >= 0.0f && phase < 1.0f);

    switch (shape)
    {
        case triangle:
            return phase > 0.5f ? 2.0f * (1.0f - phase) : 2.0f * phase;
        case sine:
            return ANON::sineTable().processSampleUnchecked (phase);
        case rampUp:
            return ANON::ramp (phase, freqHz);
        case rampDown:
            return 1.0f - ANON::ramp (phase, freqHz);
        case square:
            return ANON::square (phase, freqHz);
        case numShapes:
        default:
            jassertfalse;
            return 0.0f;
    }
}

} // namespace cgo::dsp::LfoShape
