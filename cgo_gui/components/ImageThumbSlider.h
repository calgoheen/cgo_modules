namespace cgo
{

class ImageThumbSlider : public juce::Slider
{
public:
    const int thumbWidth;
    const int thumbHeight;

    ImageThumbSlider (const juce::Image& thumb, bool isVertical);
    ~ImageThumbSlider() override;

    void paint (juce::Graphics& g) override;

private:
    const juce::Image thumb;
    const bool isVertical;

    class LookAndFeel;
    std::unique_ptr<LookAndFeel> lookAndFeel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ImageThumbSlider)
};

} // namespace cgo
