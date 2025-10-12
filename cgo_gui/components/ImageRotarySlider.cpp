#include <cgo_gui/cgo_gui.h>

namespace cgo
{

ImageRotarySlider::ImageRotarySlider (const juce::Image& img)
    : imageWidth (img.getWidth()),
    imageHeight (img.getHeight() / 2),
    backgroundImage (img.getClippedImage ({ 0, 0, imageWidth, imageHeight })),
    markerImage (img.getClippedImage ({ 0, imageHeight, imageWidth, imageHeight }))
{
    // Image height should have an even number of pixels
    jassert (img.getHeight() % 2 == 0);

    setSliderStyle (juce::Slider::SliderStyle::RotaryVerticalDrag);
    setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
}

void ImageRotarySlider::paint (juce::Graphics& g)
{
    g.drawImage (backgroundImage, getLocalBounds().toFloat());

    const auto transform = [this]
    {
        static constexpr float startAngle = juce::degreesToRadians (-130.0f);
        static constexpr float endAngle = juce::degreesToRadians (130.0f);

        const float offsetAngle = valueToProportionOfLength (getValue()) * (endAngle - startAngle);
        const juce::RectanglePlacement placement (juce::RectanglePlacement::stretchToFit);

        const auto rotate = juce::AffineTransform::rotation (startAngle + offsetAngle, 
                                                             imageWidth / 2.0f, 
                                                             imageHeight / 2.0f);

        const auto scale = placement.getTransformToFit (markerImage.getBounds().toFloat(), 
                                                        getLocalBounds().toFloat());

        return rotate.followedBy (scale);
    }();

    g.drawImageTransformed (markerImage, transform);
}

} // namespace cgo
