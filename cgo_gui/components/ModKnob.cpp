#include <cgo_gui/cgo_gui.h>

namespace cgo
{

CGO_ANON_NAMESPACE_BEGIN

constexpr float modThickness = 2.5f;
constexpr float arcThickness = 2.0f;
constexpr float discOutlineThickness = 1.0f;
constexpr float modGap = 2.0f;
constexpr int popupLift = 10;
constexpr float latchedAlpha = 0.85f;
constexpr float depthTrackAlpha = 0.4f;
constexpr float depthTickThickness = 1.5f;
constexpr float depthTickOverhang = 0.5f;
constexpr float idleAlpha = 0.75f;

constexpr float handleZone = 6.0f;
constexpr float handleWidth = 14.0f;
constexpr float handleHeight = 4.0f;

constexpr float handleHitWidth = 20.0f;
constexpr float handleHitHeight = 12.0f;

constexpr float discRatio = 0.8f;
constexpr float pointerInnerRatio = 0.4f;
constexpr float pointerOuterRatio = 0.75f;

CGO_ANON_NAMESPACE_END

bool ModKnob::Ring::operator== (const Ring& other) const
{
    return juce::approximatelyEqual (depth, other.depth) && bipolar == other.bipolar && isPreview == other.isPreview
           && depthModulated == other.depthModulated;
}

ModKnob::ModKnob() : juce::Slider (juce::Slider::RotaryVerticalDrag, juce::Slider::NoTextBox) { setWantsKeyboardFocus (false); }

ModKnob::~ModKnob()
{
    if (trackedPopup != nullptr)
        trackedPopup->removeComponentListener (this);
}

void ModKnob::paint (juce::Graphics& g)
{
    const auto [centre, modRadius, valueRadius, discRadius] = getGeometry();
    const auto rotary = getRotaryParameters();

    auto angleOf = [&rotary] (float norm)
    { return rotary.startAngleRadians + juce::jlimit (0.0f, 1.0f, norm) * (rotary.endAngleRadians - rotary.startAngleRadians); };

    auto strokeArc = [&] (float radius, float from, float to, juce::Colour colour, float thickness)
    {
        if (juce::approximatelyEqual (from, to))
            return;

        juce::Path path;
        path.addCentredArc (centre.x, centre.y, radius, radius, 0.0f, angleOf (from), angleOf (to), true);

        g.setColour (colour);
        g.strokePath (path, juce::PathStrokeType (thickness, juce::PathStrokeType::curved, juce::PathStrokeType::butt));
    };

    const float base = (float) valueToProportionOfLength (getValue());
    const auto trough = findColour (troughColourId);
    const auto modulationColour = findColour (modulationColourId);
    const auto disc = juce::Rectangle<float> (discRadius * 2.0f, discRadius * 2.0f).withCentre (centre);

    g.setColour (trough);
    g.fillEllipse (disc);
    g.setColour (findColour (discOutlineColourId));
    g.drawEllipse (disc, ANON::discOutlineThickness);

    const bool valueHot = isMouseOverOrDragging() && ! handleHover && ! draggingDepth;

    strokeArc (valueRadius, 0.0f, 1.0f, trough, ANON::arcThickness);
    strokeArc (valueRadius, 0.0f, base, findColour (valueColourId).withMultipliedAlpha (valueHot ? 1.0f : ANON::idleAlpha), ANON::arcThickness);

    const float latched = handleHover ? 1.0f : ANON::latchedAlpha;

    if (ring.has_value())
    {
        auto spanFor = [&] (float depth)
        {
            const float reach = ring->bipolar ? depth * 0.5f : depth;

            return ring->bipolar ? std::pair { base - std::abs (reach), base + std::abs (reach) }
                                 : std::pair { juce::jmin (base, base + reach), juce::jmax (base, base + reach) };
        };

        auto strokeSpan = [&] (std::pair<float, float> span, juce::Colour colour)
        { strokeArc (modRadius, juce::jlimit (0.0f, 1.0f, span.first), juce::jlimit (0.0f, 1.0f, span.second), colour, ANON::modThickness); };

        const auto colour = modulationColour.withAlpha (ring->isPreview ? unfocusedAlpha : latched);
        const auto set = spanFor (ring->depth);

        if (! ring->depthModulated)
        {
            strokeSpan (set, colour);
        }
        else
        {
            strokeSpan (set, colour.withMultipliedAlpha (ANON::depthTrackAlpha));
            strokeSpan (spanFor (liveDepth), colour);

            const float sweep = std::abs (rotary.endAngleRadians - rotary.startAngleRadians);
            const float inset = ANON::depthTickThickness * 0.5f / juce::jmax (1.0f, modRadius * sweep);

            auto notch = [&] (float end)
            {
                const auto angle = angleOf (end + (end < base ? inset : -inset));
                const float half = ANON::modThickness * 0.5f + ANON::depthTickOverhang;

                g.setColour (trough);
                g.drawLine ({ centre.getPointOnCircumference (modRadius - half, angle), centre.getPointOnCircumference (modRadius + half, angle) },
                            ANON::depthTickThickness);
            };

            if (! juce::approximatelyEqual (set.first, set.second))
            {
                if (ring->bipolar)
                {
                    notch (set.first);
                    notch (set.second);
                }
                else
                {
                    notch (ring->depth > 0.0f ? set.second : set.first);
                }
            }
        }
    }

    if (ring.has_value() || unshownModulation)
    {
        g.setColour (modulationColour.withAlpha (ring.has_value() && ! ring->isPreview ? latched : unfocusedAlpha));
        g.fillRoundedRectangle (getHandleBounds(), ANON::handleHeight * 0.5f);
    }

    if (dragOver)
        strokeArc (modRadius, 0.0f, 1.0f, modulationColour.withAlpha (0.45f), ANON::modThickness);

    const auto liveAngle = angleOf (liveValue);

    g.setColour (findColour (liveValueColourId));
    g.drawLine ({ centre.getPointOnCircumference (valueRadius * ANON::pointerInnerRatio, liveAngle),
                  centre.getPointOnCircumference (valueRadius * ANON::pointerOuterRatio, liveAngle) },
                ANON::arcThickness);
}

void ModKnob::mouseDown (const juce::MouseEvent& e)
{
    if (e.mods.isPopupMenu() && onRightClick != nullptr)
    {
        onRightClick();
        return;
    }

    if (handleIsGrabbable() && hitsHandle (e.position))
    {
        setHandleHover (true);

        draggingDepth = true;
        depthAtDragStart = ring->depth;

        if (onDepthGestureStart != nullptr)
            onDepthGestureStart();

        return;
    }

    juce::Slider::mouseDown (e);
    liftPopupDisplay();
}

void ModKnob::mouseDrag (const juce::MouseEvent& e)
{
    if (! draggingDepth)
    {
        juce::Slider::mouseDrag (e);
        liftPopupDisplay();
        return;
    }

    if (! ring.has_value())
        return;

    const float travel = juce::jmax (1, getMouseDragSensitivity());
    const float delta = -(float) e.getDistanceFromDragStartY() / travel * 2.0f;
    const float depth = juce::jlimit (-1.0f, 1.0f, depthAtDragStart + delta);

    if (juce::approximatelyEqual (ring->depth, depth))
        return;

    ring->depth = depth;
    repaint();

    onDepthChanged (depth);
}

void ModKnob::mouseUp (const juce::MouseEvent& e)
{
    if (! draggingDepth)
    {
        juce::Slider::mouseUp (e);
        return;
    }

    draggingDepth = false;
    setHandleHover (handleIsGrabbable() && hitsHandle (e.position));

    if (onDepthGestureEnd != nullptr)
        onDepthGestureEnd();
}

void ModKnob::mouseEnter (const juce::MouseEvent& e)
{
    juce::Slider::mouseEnter (e);
    setHandleHover (handleIsGrabbable() && hitsHandle (e.position));
}

void ModKnob::mouseMove (const juce::MouseEvent& e)
{
    juce::Slider::mouseMove (e);
    setHandleHover (handleIsGrabbable() && hitsHandle (e.position));
}

void ModKnob::mouseExit (const juce::MouseEvent& e)
{
    juce::Slider::mouseExit (e);
    setHandleHover (false);
}

bool ModKnob::isInterestedInDragSource (const SourceDetails&) { return acceptsDrops; }

void ModKnob::itemDragEnter (const SourceDetails&)
{
    dragOver = true;
    repaint();
}

void ModKnob::itemDragExit (const SourceDetails&)
{
    dragOver = false;
    repaint();
}

void ModKnob::itemDropped (const SourceDetails& details)
{
    dragOver = false;
    repaint();

    if (onDrop != nullptr)
        onDrop (details.description);
}

void ModKnob::setModulation (std::optional<Ring> newRing, bool hasUnshownModulation)
{
    if (ring == newRing && unshownModulation == hasUnshownModulation)
        return;

    ring = newRing;
    unshownModulation = hasUnshownModulation;
    repaint();

    setHandleHover (handleIsGrabbable() && isMouseOver() && hitsHandle (getMouseXYRelative().toFloat()));
}

void ModKnob::setLiveValue (float normalisedValue)
{
    const auto clamped = juce::jlimit (0.0f, 1.0f, normalisedValue);

    if (juce::approximatelyEqual (liveValue, clamped))
        return;

    liveValue = clamped;
    repaint();
}

void ModKnob::setLiveDepth (float depth)
{
    const auto clamped = juce::jlimit (-1.0f, 1.0f, depth);

    if (juce::approximatelyEqual (liveDepth, clamped))
        return;

    liveDepth = clamped;

    if (ring.has_value() && ring->depthModulated)
        repaint();
}

void ModKnob::setAcceptsDrops (bool shouldAccept)
{
    if (acceptsDrops == shouldAccept)
        return;

    acceptsDrops = shouldAccept;

    if (! acceptsDrops && dragOver)
    {
        dragOver = false;
        repaint();
    }
}

void ModKnob::componentMovedOrResized (juce::Component&, bool wasMoved, bool)
{
    if (wasMoved)
        liftPopupDisplay();
}

void ModKnob::componentBeingDeleted (juce::Component& component)
{
    component.removeComponentListener (this);
    trackedPopup = nullptr;
    liftedPopupY.reset();
}

void ModKnob::setHandleHover (bool isOverHandle)
{
    if (handleHover == isOverHandle)
        return;

    handleHover = isOverHandle;
    repaint();
}

void ModKnob::liftPopupDisplay()
{
    auto* popup = getCurrentPopupDisplay();

    if (popup != trackedPopup)
    {
        if (trackedPopup != nullptr)
            trackedPopup->removeComponentListener (this);

        trackedPopup = popup;
        liftedPopupY.reset();

        if (trackedPopup != nullptr)
            trackedPopup->addComponentListener (this);
    }

    if (popup == nullptr || liftedPopupY == popup->getY())
        return;

    liftedPopupY = popup->getY() - ANON::popupLift;
    popup->setTopLeftPosition (popup->getX(), *liftedPopupY);
}

ModKnob::Geometry ModKnob::getGeometry() const
{
    const auto area = getLocalBounds().toFloat().reduced (2.0f).withTrimmedBottom (ANON::handleZone);
    const float outer = juce::jmin (area.getWidth(), area.getHeight()) * 0.5f;

    const float modRadius = outer - ANON::modThickness * 0.5f;
    const float valueRadius = modRadius - ANON::modThickness * 0.5f - ANON::modGap - ANON::arcThickness * 0.5f;

    return { area.getCentre(), modRadius, valueRadius, valueRadius * ANON::discRatio };
}

bool ModKnob::handleIsGrabbable() const { return ring.has_value() && ! ring->isPreview && onDepthChanged != nullptr; }

juce::Rectangle<float> ModKnob::getHandleBounds() const
{
    const auto area = getLocalBounds().toFloat().reduced (2.0f);

    return juce::Rectangle<float> (ANON::handleWidth, ANON::handleHeight).withCentre ({ area.getCentreX(), area.getBottom() - ANON::handleZone * 0.5f });
}

bool ModKnob::hitsHandle (juce::Point<float> position) const
{
    return getHandleBounds().withSizeKeepingCentre (ANON::handleHitWidth, ANON::handleHitHeight).contains (position);
}

} // namespace cgo
