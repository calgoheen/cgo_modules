namespace cgo
{

class ImageToggleButton : public juce::Button
{
public:
    const int imageWidth;
    const int imageHeight;

    ImageToggleButton (const juce::Image& img);
    ~ImageToggleButton() override = default;

    void paintButton (juce::Graphics& g, bool, bool) override;

private:
    const juce::Image offImage;
    const juce::Image onImage;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ImageToggleButton)
};

} // namespace cgo
