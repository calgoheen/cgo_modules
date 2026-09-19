#include <cgo_plugin/cgo_plugin.h>

namespace cgo
{

static juce::PropertiesFile::Options getPropertiesFileOptions()
{
    juce::PropertiesFile::Options options;

#if defined(JucePlugin_Name) && defined(JucePlugin_Manufacturer)
    options.applicationName = JucePlugin_Name;
    options.folderName = JucePlugin_Manufacturer;
#else
    jassertfalse;
#endif

    options.filenameSuffix = ".settings";
    options.osxLibrarySubFolder = "Application Support";

    return options;
}

juce::Component* ResizableEditor::getContentComponent (const juce::Component* descendant)
{
    if (descendant == nullptr)
        return nullptr;

    if (auto* editor = descendant->findParentComponentOfClass<ResizableEditor>())
        return editor->component.get();

    return nullptr;
}

ResizableEditor::ResizableEditor (juce::AudioProcessor& proc, std::unique_ptr<juce::Component> comp, float defaultScale, float minScale, float maxScale)
  : AudioProcessorEditor (proc),
    component (std::move (comp)),
    baseWidth (component->getWidth()),
    baseHeight (component->getHeight()),
    defaultScaleFactor (defaultScale),
    minScaleFactor (minScale),
    maxScaleFactor (maxScale),
    propertiesFile (getPropertiesFileOptions())
{
    jassert (minScaleFactor > 0.0f && minScaleFactor <= maxScaleFactor);

    addAndMakeVisible (*component);

    const double scale = loadEditorScaleFactor();
    setSize (juce::roundToInt (baseWidth * scale), juce::roundToInt (baseHeight * scale));

    setResizable (false, true);
    setResizeLimits (juce::roundToInt (baseWidth * minScaleFactor),
                     juce::roundToInt (baseHeight * minScaleFactor),
                     juce::roundToInt (baseWidth * maxScaleFactor),
                     juce::roundToInt (baseHeight * maxScaleFactor));
    getConstrainer()->setFixedAspectRatio (baseWidth / (double) baseHeight);
}

ResizableEditor::~ResizableEditor() { propertiesFile.saveIfNeeded(); }

void ResizableEditor::resized()
{
    const float scale = getWidth() / (float) baseWidth;
    component->setTransform (juce::AffineTransform::scale (scale, scale));
    saveEditorScaleFactor (scale);
}

double ResizableEditor::loadEditorScaleFactor() const
{
    return juce::jlimit ((double) minScaleFactor, (double) maxScaleFactor, propertiesFile.getDoubleValue ("scale", defaultScaleFactor));
}

void ResizableEditor::saveEditorScaleFactor (double scale) { propertiesFile.setValue ("scale", scale); }

} // namespace cgo
