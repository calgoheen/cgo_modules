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

ResizableEditor::ResizableEditor (juce::AudioProcessor& proc, 
                                  std::unique_ptr<juce::Component> comp, 
                                  float defaultScale)
    : AudioProcessorEditor (proc),
    component (std::move (comp)),
    maxWidth (component->getWidth()),
    maxHeight (component->getHeight()),
    defaultScaleFactor (defaultScale),
    propertiesFile (getPropertiesFileOptions())
{
    addAndMakeVisible (*component);

    const float scale = loadEditorScaleFactor();
    setSize (maxWidth * scale, maxHeight * scale);

    setResizable (false, true);
    setResizeLimits (maxWidth / 4.0, maxHeight / 4.0, maxWidth, maxHeight);
    getConstrainer()->setFixedAspectRatio (maxWidth / static_cast<double> (maxHeight));
}

ResizableEditor::~ResizableEditor()
{
    propertiesFile.saveIfNeeded();
}

void ResizableEditor::resized()
{
    const float scale = getWidth() / static_cast<float> (maxWidth);
    component->setTransform (juce::AffineTransform::scale (scale, scale));
    saveEditorScaleFactor (scale);
}

double ResizableEditor::loadEditorScaleFactor() const
{
    return propertiesFile.getDoubleValue ("scale", defaultScaleFactor);
}

void ResizableEditor::saveEditorScaleFactor (double scale)
{
    propertiesFile.setValue ("scale", scale);
}

} // namespace cgo
