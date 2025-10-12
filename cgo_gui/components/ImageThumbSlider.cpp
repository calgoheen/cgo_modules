#include <cgo_gui/cgo_gui.h>

namespace cgo
{

class ImageThumbSlider::LookAndFeel : public juce::LookAndFeel_V4
{
public:
    LookAndFeel (int radius) : thumbRadius (radius) {}
    ~LookAndFeel() override = default;

    int getSliderThumbRadius (juce::Slider&) override
    {
        return thumbRadius;
    }

private:
    const int thumbRadius;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LookAndFeel)
};

ImageThumbSlider::ImageThumbSlider (const juce::Image& img, bool vertical)
    : thumbWidth (img.getWidth()), 
    thumbHeight (img.getHeight()), 
    thumb (img),
    isVertical (vertical)
{
    setSliderStyle (isVertical ? juce::Slider::SliderStyle::LinearVertical : juce::Slider::SliderStyle::LinearHorizontal);
    setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);

    lookAndFeel = std::make_unique<LookAndFeel> (isVertical ? thumbHeight / 2 : thumbWidth / 2);
    setLookAndFeel (lookAndFeel.get());
}

ImageThumbSlider::~ImageThumbSlider()
{
    setLookAndFeel (nullptr);
}

void ImageThumbSlider::paint (juce::Graphics& g)
{
    const auto thumbBounds = [this] 
    {
        const float relativePos = valueToProportionOfLength (getValue());

        if (isVertical)
        {
            const float yMax = getHeight() - thumbHeight;

            return juce::Rectangle<float> (0.0f,
                                           juce::jmap (relativePos, yMax, 0.0f),
                                           thumbWidth,
                                           thumbHeight);
        }
        else
        {
            const float xMax = getWidth() - thumbWidth;
            
            return juce::Rectangle<float> (juce::jmap (relativePos, 0.0f, xMax),
                                           0.0f,
                                           thumbWidth,
                                           thumbHeight);
        }
    }();
    
    g.drawImage (thumb, thumbBounds);
}

} // namespace cgo
