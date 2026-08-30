namespace cgo
{

class ModulatedParameter : private ModulatedValue, private juce::AudioProcessorParameter::Listener
{
public:
    using ModulatedValue::range;

    using ModulatedValue::getBuffer;
    using ModulatedValue::getCurrentValue;
    using ModulatedValue::isChanging;
    using ModulatedValue::wasTouched;

    ~ModulatedParameter() override;

    juce::RangedAudioParameter& parameter;

private:
    ModulatedParameter (juce::RangedAudioParameter& parameter, double smoothing, Rate rate);

    void parameterValueChanged (int, float newValue) override;
    void parameterGestureChanged (int, bool) override;

    void snapToParameter();

    friend class ParameterOwner;

    JUCE_DECLARE_NON_COPYABLE (ModulatedParameter)
};

} // namespace cgo
