namespace cgo
{

class ResizableEditor : public juce::AudioProcessorEditor
{
public:
    ResizableEditor (juce::AudioProcessor& processor, 
                     std::unique_ptr<juce::Component> component, 
                     float defaultScaleFactor = 0.5f);

    ~ResizableEditor() override;

    void resized() override;

private:
    double loadEditorScaleFactor() const;
    void saveEditorScaleFactor (double scale);

    std::unique_ptr<juce::Component> component;

    const int maxWidth;
    const int maxHeight;
    const float defaultScaleFactor;

    juce::PropertiesFile propertiesFile;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ResizableEditor)
};

} // namespace cgo
