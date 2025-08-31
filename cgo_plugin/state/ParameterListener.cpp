#include <cgo_plugin/cgo_plugin.h>

namespace cgo
{

ParameterListener::ParameterListener (juce::RangedAudioParameter& param, std::function<void (float)> onChange)
    : parameter (param),
      callback (std::move (onChange))
{
    jassert (callback != nullptr);

    parameter.addListener (this);
    sendUpdate();
}

ParameterListener::~ParameterListener()
{
    parameter.removeListener (this);
}

void ParameterListener::sendUpdate() const
{
    callback (parameter.getValue());
}

void ParameterListener::parameterValueChanged (int, float newValue)
{
    callback (newValue);
}

void ParameterListener::parameterGestureChanged (int, bool)
{
}

} // namespace cgo
