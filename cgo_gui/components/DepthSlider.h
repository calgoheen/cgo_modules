namespace cgo
{

class DepthSlider : public juce::Component
{
public:
    enum ColourIds
    {
        troughColourId = 0x1e00110,
        valueColourId
    };

    DepthSlider();

    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseUp (const juce::MouseEvent& e) override;
    void mouseDoubleClick (const juce::MouseEvent& e) override;

    void setRange (float newMinimum, float newMaximum);
    void setValue (float newValue);
    float getValue() const { return value; }

    std::function<void (float value)> onValueChange;
    std::function<void()> onDragStart;
    std::function<void()> onDragEnd;

private:
    float xForValue (float v) const;
    float originValue() const;
    void dragTo (const juce::MouseEvent& e);

    float minimum = -1.0f;
    float maximum = 1.0f;
    float value = 0.0f;
    bool dragging = false;

    JUCE_DECLARE_NON_COPYABLE (DepthSlider)
};

} // namespace cgo
