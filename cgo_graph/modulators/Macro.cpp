#include <cgo_graph/cgo_graph.h>

namespace cgo
{

Macro::Macro()
  : params { .value = addModulatedParameter (ParamUtils::createPercentParameter ("value", "Value", 0.0f), {}, ModulatedValue::Rate::audio) }
{
}

juce::String Macro::getTypeId() const { return "macro"; }

void Macro::prepareImpl() {}

void Macro::processImpl (float* buffer, int numSamples) { juce::FloatVectorOperations::copy (buffer, params.value.getBuffer(), numSamples); }

} // namespace cgo
