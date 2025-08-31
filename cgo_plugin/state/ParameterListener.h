namespace cgo
{

class ParameterListener : public juce::AudioProcessorParameter::Listener
{
public:
    ParameterListener (juce::RangedAudioParameter& param, std::function<void (float)> onChange);
    ~ParameterListener() override;

    void sendUpdate() const;

    juce::RangedAudioParameter& parameter;
    const std::function<void (float)> callback { nullptr };

private:
    void parameterValueChanged (int, float newValue) override;
    void parameterGestureChanged (int, bool) override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ParameterListener)
};

} // namespace cgo
