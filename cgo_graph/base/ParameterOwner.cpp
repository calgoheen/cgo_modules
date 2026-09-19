#include <cgo_graph/cgo_graph.h>

namespace cgo
{

namespace paramIds
{

const juce::Identifier PARAMS { "PARAMS" };
const juce::Identifier P { "P" };
const juce::Identifier id { "id" };
const juce::Identifier v { "v" };

} // namespace paramIds

class ParameterHostProcessor final : public BasicAudioProcessor
{
public:
    ParameterHostProcessor() : BasicAudioProcessor (BusesProperties(), false) {}
    void prepareToPlay (double, int) override {}
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override {}

protected:
    bool isBusesLayoutSupported (const BusesLayout&) const override { return true; }
};

ParameterOwner::ParameterOwner() : parameterHost (std::make_unique<ParameterHostProcessor>()) {}

const juce::OwnedArray<ModulatedParameter>& ParameterOwner::getModulatedParameters() const { return modulatedParameters; }

int ParameterOwner::indexOf (const ModulatedParameter& param) const { return modulatedParameters.indexOf (&param); }

int ParameterOwner::indexOf (const juce::String& paramID) const
{
    for (int i = 0; i < modulatedParameters.size(); i++)
        if (modulatedParameters[i]->parameter.getParameterID() == paramID)
            return i;

    return -1;
}

void ParameterOwner::snapParameterValues()
{
    for (auto* mp : modulatedParameters)
        mp->snapToParameter();
}

juce::ValueTree ParameterOwner::parametersToValueTree() const
{
    juce::ValueTree tree { paramIds::PARAMS };

    for (auto* mp : modulatedParameters)
    {
        juce::ValueTree p { paramIds::P };

        p.setProperty (paramIds::id, mp->parameter.getParameterID(), nullptr);
        p.setProperty (paramIds::v, mp->parameter.getValue(), nullptr);

        tree.appendChild (p, nullptr);
    }

    return tree;
}

void ParameterOwner::restoreParameters (const juce::ValueTree& nodeTree) { restoreParameterValues (nodeTree.getChildWithName (paramIds::PARAMS)); }

void ParameterOwner::restoreParameterValues (const juce::ValueTree& paramsTree)
{
    jassert (! paramsTree.isValid() || paramsTree.hasType (paramIds::PARAMS));

    for (const auto p : paramsTree)
    {
        const int index = indexOf (p[paramIds::id].toString());

        // The node no longer has a parameter with this ID
        if (index < 0)
        {
            jassertfalse;
            continue;
        }

        modulatedParameters[index]->parameter.setValueNotifyingHost ((float) p[paramIds::v]);
    }

    snapParameterValues();
}

std::vector<std::optional<float>> ParameterOwner::savedParameterValues (const juce::ValueTree& nodeTree) const
{
    std::vector<std::optional<float>> values ((size_t) modulatedParameters.size());

    for (const auto p : nodeTree.getChildWithName (paramIds::PARAMS))
    {
        const int index = indexOf (p[paramIds::id].toString());

        // The node no longer has a parameter with this ID
        if (index >= 0)
            values[(size_t) index] = (float) p[paramIds::v];
    }

    return values;
}

void ParameterOwner::prepareParameters (double sampleRate, int blockSize)
{
    for (auto* mp : modulatedParameters)
        mp->prepare (sampleRate, blockSize);
}

void ParameterOwner::processParameters (int numSamples, const NodeModulation* modulation)
{
    const NodeModulation unmodulated;
    const auto& mod = modulation != nullptr ? *modulation : unmodulated;

    for (int i = 0; i < modulatedParameters.size(); i++)
        modulatedParameters[i]->process (numSamples, mod.viewFor (i));
}

void ParameterOwner::markParametersUnsettled()
{
    for (auto* mp : modulatedParameters)
        mp->markUnsettled();
}

juce::RangedAudioParameter& ParameterOwner::hostParameter (std::unique_ptr<juce::RangedAudioParameter> param)
{
    // Parameter IDs must be unique within an owner
    jassert (indexOf (param->getParameterID()) < 0);

    auto* raw = param.release();
    parameterHost->addParameter (raw);
    return *raw;
}

} // namespace cgo
