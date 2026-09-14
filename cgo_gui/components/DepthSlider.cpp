#include <cgo_gui/cgo_gui.h>

namespace cgo
{

CGO_ANON_NAMESPACE_BEGIN

constexpr float barHeight = 6.0f;
constexpr float centreTickOverhang = 3.0f;
constexpr float centreTickThickness = 1.0f;
constexpr float centreTickAlpha = 0.35f;

CGO_ANON_NAMESPACE_END

DepthSlider::DepthSlider() { setWantsKeyboardFocus (false); }

void DepthSlider::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    const auto track = juce::Rectangle<float> (bounds.getX(), bounds.getCentreY() - ANON::barHeight * 0.5f, bounds.getWidth(), ANON::barHeight);

    const float originX = xForValue (originValue());
    const float valueX = xForValue (value);
    const auto fill = findColour (valueColourId);

    g.setColour (findColour (troughColourId));
    g.fillRect (track);

    // Only worth marking the origin when the fill can sit either side of it
    if (originValue() > minimum && originValue() < maximum)
    {
        g.setColour (fill.withAlpha (ANON::centreTickAlpha));
        g.fillRect (
            juce::Rectangle<float> (ANON::centreTickThickness, track.getHeight() + 2.0f * ANON::centreTickOverhang).withCentre ({ originX, track.getCentreY() }));
    }

    g.setColour (fill);
    g.fillRect (track.withLeft (juce::jmin (originX, valueX)).withRight (juce::jmax (originX, valueX)));
}

void DepthSlider::mouseDown (const juce::MouseEvent& e)
{
    dragging = true;

    if (onDragStart != nullptr)
        onDragStart();

    dragTo (e);
}

void DepthSlider::mouseDrag (const juce::MouseEvent& e) { dragTo (e); }

void DepthSlider::mouseUp (const juce::MouseEvent&)
{
    if (! dragging)
        return;

    dragging = false;

    if (onDragEnd != nullptr)
        onDragEnd();
}

void DepthSlider::mouseDoubleClick (const juce::MouseEvent&)
{
    if (juce::approximatelyEqual (value, originValue()))
        return;

    setValue (originValue());

    if (onValueChange != nullptr)
        onValueChange (value);
}

void DepthSlider::setRange (float newMinimum, float newMaximum)
{
    jassert (newMinimum < newMaximum);

    minimum = newMinimum;
    maximum = newMaximum;

    setValue (value);
    repaint();
}

void DepthSlider::setValue (float newValue)
{
    newValue = juce::jlimit (minimum, maximum, newValue);

    if (juce::approximatelyEqual (value, newValue))
        return;

    value = newValue;
    repaint();
}

float DepthSlider::xForValue (float v) const
{
    return juce::jmap (juce::jlimit (minimum, maximum, v), minimum, maximum, 0.0f, (float) getWidth());
}

float DepthSlider::originValue() const { return juce::jlimit (minimum, maximum, 0.0f); }

void DepthSlider::dragTo (const juce::MouseEvent& e)
{
    if (getWidth() <= 0)
        return;

    const float newValue = juce::jmap ((float) e.position.x, 0.0f, (float) getWidth(), minimum, maximum);

    if (juce::approximatelyEqual (value, juce::jlimit (minimum, maximum, newValue)))
        return;

    setValue (newValue);

    if (onValueChange != nullptr)
        onValueChange (value);
}

} // namespace cgo
