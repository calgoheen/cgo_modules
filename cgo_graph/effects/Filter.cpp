#include <cgo_graph/cgo_graph.h>

namespace cgo
{

Filter::Filter()
  : Processor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true).withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
    params { .order = addModulatedParameter (ParamUtils::createChoiceParameter ("order", "Order", { "12 dB", "24 dB" }, 0)),
             .cutoff = addModulatedParameter (ParamUtils::createFreqParameter ("cutoff", "Cutoff", 20.0f, 20e3f, 1e3f, 1e3f)),
             .resonance =
                 addModulatedParameter (ParamUtils::createRangedParameter ("res", "Resonance", "", ParamUtils::getRangeWithCenter (0.1f, 10.0f, 1.0f), 0.71f)),
             .type = addModulatedParameter (ParamUtils::createChoiceParameter ("type", "Type", { "Lowpass", "Bandpass", "Highpass", "Notch" }, 0)) }
{
}

juce::String Filter::getTypeId() const { return "filter"; }

void Filter::prepareImpl() { filter.emplace ((float) getSampleRate(), getNumChannels()); }

void Filter::processImpl (juce::AudioBuffer<float>& buffer)
{
    if (anyChanging (params.type, params.order))
        updateParameters();

    const auto channels = buffer.getArrayOfWritePointers();
    const int numSamples = buffer.getNumSamples();

    if (! anyChanging (params.cutoff, params.resonance))
    {
        filter->process (channels, 0, numSamples);
        return;
    }

    for (int i = 0; i < numSamples; i++)
    {
        filter->setFilter (params.cutoff.getBuffer()[i], params.resonance.getBuffer()[i]);
        filter->process (channels, i, 1);
    }
}

void Filter::resetImpl() { filter->reset(); }

void Filter::updateParameters()
{
    filter->setType (ParamUtils::choiceFromValue<dsp::Filter::Type> (params.type.getCurrentValue()));
    filter->setOrder (ParamUtils::choiceFromValue<dsp::Filter::Order> (params.order.getCurrentValue()));
}

} // namespace cgo
