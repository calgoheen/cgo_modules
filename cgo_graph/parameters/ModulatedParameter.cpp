#include <cgo_graph/cgo_graph.h>

namespace cgo
{

ModulatedParameter::~ModulatedParameter() { parameter.removeListener (this); }

ModulatedParameter::ModulatedParameter (juce::RangedAudioParameter& p, double smoothing, Rate rate)
  : ModulatedValue (p.getNormalisableRange(), smoothing, rate), parameter (p)
{
    snapToNormalized (p.getValue());
    parameter.addListener (this);
}

void ModulatedParameter::parameterValueChanged (int, float newValue) { setValueNormalized (newValue); }

void ModulatedParameter::parameterGestureChanged (int, bool) {}

void ModulatedParameter::snapToParameter() { snapToNormalized (parameter.getValue()); }

} // namespace cgo
