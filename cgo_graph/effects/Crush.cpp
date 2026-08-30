#include <cgo_graph/cgo_graph.h>

namespace cgo
{

Crush::Crush()
  : Processor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true).withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
    params { .downsample = addModulatedParameter (ParamUtils::createRangedParameter (
                 "downsample",
                 "Downsample",
                 "x",
                 ParamUtils::getRangeWithCenter (1.0f, 100.0f, 10.0f),
                 1.0f,
                 [] (float val, int) { return juce::String (val, 2); },
                 [] (const juce::String& str) { return str.getFloatValue(); })),
             .bitDepth = addModulatedParameter (ParamUtils::createRangedParameter (
                 "bit_depth",
                 "Bit Depth",
                 "bits",
                 { 1.0f, 16.0f },
                 16.0f,
                 [] (float val, int) { return juce::String (val, 1); },
                 [] (const juce::String& str) { return str.getFloatValue(); })) }
{
}

juce::String Crush::getTypeId() const { return "crush"; }

void Crush::prepareImpl() { crush.emplace (getNumChannels()); }

void Crush::processImpl (juce::AudioBuffer<float>& buffer)
{
    const auto channels = buffer.getArrayOfWritePointers();
    const int numSamples = buffer.getNumSamples();

    if (! anyChanging (params.downsample, params.bitDepth))
    {
        crush->process (channels, 0, numSamples);
        return;
    }

    for (int i = 0; i < numSamples; i++)
    {
        if (params.downsample.isChanging())
            crush->setDownsampleFactor (params.downsample.getBuffer()[i]);

        if (params.bitDepth.isChanging())
            crush->setBitDepth (params.bitDepth.getBuffer()[i]);

        crush->process (channels, i, 1);
    }
}

} // namespace cgo
