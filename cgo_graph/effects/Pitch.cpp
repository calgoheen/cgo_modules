#include <cgo_graph/cgo_graph.h>

namespace cgo
{

Pitch::Pitch()
  : Processor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true).withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
    params { .pitch = addModulatedParameter (
                 ParamUtils::createRangedParameter ("pitch",
                                                    "Pitch",
                                                    "st",
                                                    { -24.0f, 24.0f },
                                                    0.0f,
                                                    [] (float x, int) { return juce::String (x, 2); },
                                                    [] (const juce::String& str) { return str.getFloatValue(); })),
             .mix = addModulatedParameter (ParamUtils::createPercentParameter ("mix", "Mix", 1.0f)) }
{
}

juce::String Pitch::getTypeId() const { return "pitch"; }

void Pitch::prepareImpl() { shifter.emplace (getNumChannels()); }

void Pitch::processImpl (juce::AudioBuffer<float>& buffer)
{
    const auto channels = buffer.getArrayOfWritePointers();
    const int numSamples = buffer.getNumSamples();

    if (! anyChanging (params.pitch, params.mix))
    {
        shifter->process (channels, 0, numSamples);
        return;
    }

    for (int i = 0; i < numSamples; i++)
    {
        if (params.pitch.isChanging())
            shifter->setShiftSemitones (params.pitch.getBuffer()[i]);

        if (params.mix.isChanging())
            shifter->setMix (params.mix.getBuffer()[i]);

        shifter->process (channels, i, 1);
    }
}

} // namespace cgo
