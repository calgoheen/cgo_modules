namespace cgo
{

class ParameterOwner
{
public:
    ParameterOwner();
    virtual ~ParameterOwner() = default;

    const juce::OwnedArray<ModulatedParameter>& getModulatedParameters() const;
    int indexOf (const ModulatedParameter& param) const;
    int indexOf (const juce::String& paramID) const;

    void snapParameterValues();

    juce::ValueTree parametersToValueTree() const;
    void restoreParameters (const juce::ValueTree& nodeTree);
    void restoreParameterValues (const juce::ValueTree& paramsTree);
    std::vector<std::optional<float>> savedParameterValues (const juce::ValueTree& nodeTree) const;

protected:
    static constexpr double noSmoothing = 0.0;

    template <typename ParamType>
    ModulatedParameter& addModulatedParameter (std::unique_ptr<ParamType> param,
                                               std::optional<double> smoothingSeconds = {},
                                               ModulatedValue::Rate rate = ModulatedValue::Rate::block)
    {
        static_assert (std::is_base_of_v<juce::AudioParameterChoice, ParamType> || std::is_base_of_v<juce::AudioParameterFloat, ParamType>,
                       "Invalid ParamType");
        jassert (param != nullptr);
        jassert (! smoothingSeconds.has_value() || *smoothingSeconds >= 0.0);

        constexpr double defaultSmoothing = std::is_base_of_v<juce::AudioParameterChoice, ParamType> ? 0.0 : 50e-3;

        auto& hosted = hostParameter (std::move (param));
        return *modulatedParameters.add (new ModulatedParameter (hosted, smoothingSeconds.value_or (defaultSmoothing), rate));
    }

    void prepareParameters (double sampleRate, int blockSize);
    void processParameters (int numSamples, const NodeModulation* modulation = nullptr);

private:
    juce::RangedAudioParameter& hostParameter (std::unique_ptr<juce::RangedAudioParameter> param);

    std::unique_ptr<juce::AudioProcessor> parameterHost;
    juce::OwnedArray<ModulatedParameter> modulatedParameters;

    JUCE_DECLARE_NON_COPYABLE (ParameterOwner)
};

} // namespace cgo
