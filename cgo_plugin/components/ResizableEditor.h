namespace cgo
{

class ResizableEditor : public juce::AudioProcessorEditor
{
public:
    ResizableEditor (juce::AudioProcessor& processor,
                     std::unique_ptr<juce::Component> component,
                     float defaultScaleFactor = 1.0f,
                     float minScaleFactor = 0.5f,
                     float maxScaleFactor = 2.0f);

    ~ResizableEditor() override;

    void resized() override;

private:
    double loadEditorScaleFactor() const;
    void saveEditorScaleFactor (double scale);

    std::unique_ptr<juce::Component> component;

    const int baseWidth;
    const int baseHeight;
    const float defaultScaleFactor;
    const float minScaleFactor;
    const float maxScaleFactor;

    juce::PropertiesFile propertiesFile;

    JUCE_DECLARE_NON_COPYABLE (ResizableEditor)
};

} // namespace cgo
