namespace cgo
{

class ImageRotarySlider : public juce::Slider
{
public:
    const int imageWidth;
    const int imageHeight;

    ImageRotarySlider (const juce::Image& img);
    ~ImageRotarySlider() override = default;

    void paint (juce::Graphics& g) override;

private:
    const juce::Image backgroundImage;
    const juce::Image markerImage;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ImageRotarySlider)
};

} // namespace cgo
