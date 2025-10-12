#include <cgo_gui/cgo_gui.h>

namespace cgo
{

FilmStripSlider::FilmStripSlider (const juce::Image& img, int frames)
    : frameWidth (img.getWidth()), 
    frameHeight (img.getHeight() / frames), 
    filmStrip (img),
    numFrames (frames)
{
    setSliderStyle (juce::Slider::SliderStyle::RotaryVerticalDrag);
    setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
}

void FilmStripSlider::paint (juce::Graphics& g)
{
    const int frame = static_cast<int> (valueToProportionOfLength (getValue()) * (numFrames - 1));

    g.drawImage (filmStrip.getClippedImage ({ 0, frame * frameHeight, frameWidth, frameHeight }),
                 getLocalBounds().toFloat());
}

} // namespace cgo
