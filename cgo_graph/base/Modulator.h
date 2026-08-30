namespace cgo
{

struct ModulatorAudio
{
    const juce::AudioBuffer<float>* input = nullptr;
    const juce::AudioBuffer<float>* sidechain = nullptr;
};

class Modulator : public ParameterOwner, public juce::ReferenceCountedObject, protected PlayHeadState
{
public:
    using Ptr = juce::ReferenceCountedObjectPtr<Modulator>;

    Modulator() = default;

    virtual juce::String getTypeId() const = 0;

    void prepare (double sampleRate, int maxBlockSize);
    void process (int numSamples, const NodeModulation* modulation = nullptr, const ModulatorAudio& audio = {});
    void setPlayHead (juce::AudioPlayHead* newPlayHead);

    const float* getBuffer() const { return buffer.read(); }

protected:
    virtual void prepareImpl() = 0;
    virtual void processImpl (float* buffer, int numSamples) = 0;

    double getSampleRate() const;
    int getBlockSize() const;
    const ModulatorAudio& getAudio() const;

private:
    ControlBuffer buffer;

    juce::AudioPlayHead* playHead { nullptr };
    double sampleRate { 0.0 };
    int blockSize { 0 };
    ModulatorAudio audio;

    JUCE_DECLARE_NON_COPYABLE (Modulator)
};

} // namespace cgo
