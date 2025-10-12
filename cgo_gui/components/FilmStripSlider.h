namespace cgo
{

class FilmStripSlider : public juce::Slider
{
public:
    const int frameWidth;
    const int frameHeight;
    
    FilmStripSlider (const juce::Image& filmStrip, int numFrames);
    ~FilmStripSlider() override = default;

    void paint (juce::Graphics& g) override;

private:
    const juce::Image filmStrip;
    const int numFrames;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FilmStripSlider)
};

} // namespace cgo
