#include <cgo_gui/cgo_gui.h>

namespace cgo
{

ImageToggleButton::ImageToggleButton (const juce::Image& img) 
    : Button (""),
    imageWidth (img.getWidth()),
    imageHeight (img.getHeight() / 2),
    offImage (img.getClippedImage ({ 0, 0, imageWidth, imageHeight })),
    onImage (img.getClippedImage ({ 0, imageHeight, imageWidth, imageHeight }))
{
    // Image height should have an even number of pixels
    jassert (img.getHeight() % 2 == 0);

    setClickingTogglesState (true);
}

void ImageToggleButton::paintButton (juce::Graphics& g, bool, bool)
{
    g.drawImage (getToggleState() ? onImage : offImage, getLocalBounds().toFloat());
}

} // namespace cgo
