namespace cgo
{

class ControlBuffer
{
public:
    class Writer
    {
    public:
        Writer (ControlBuffer& b, int numSamplesToWrite) : buffer (b), ptr (b.buffer.getWritePointer (0)), numSamples (numSamplesToWrite) {}
        ~Writer() { buffer.holdTail (numSamples); }

        float* data() const { return ptr; }
        float& operator[] (int i) const { return ptr[i]; }

    private:
        ControlBuffer& buffer;
        float* const ptr;
        const int numSamples;
    };

    ControlBuffer() = default;

    void prepare (int maxBlockSize)
    {
        buffer.setSize (1, maxBlockSize);
        buffer.clear();
    }

    /** Takes the buffer for a block of numSamples. */
    Writer write (int numSamples) { return { *this, numSamples }; }

    /** Returns a pointer for read-only access. */
    const float* read() const { return buffer.getReadPointer (0); }

    /** Writes a linear ramp across numSamples, ending on `to`. */
    void writeRamp (float from, float to, int numSamples)
    {
        if (numSamples <= 0)
            return;

        const Writer block (*this, numSamples);
        const float delta = (to - from) / (float) numSamples;

        for (int i = 0; i < numSamples; i++)
            block[i] = from + (float) (i + 1) * delta;

        block[numSamples - 1] = to;
    }

    /** Writes one value across the whole buffer. */
    void writeConstant (float value) { juce::FloatVectorOperations::fill (buffer.getWritePointer (0), value, buffer.getNumSamples()); }

private:
    void holdTail (int numWritten)
    {
        const int total = buffer.getNumSamples();

        if (numWritten <= 0 || numWritten >= total)
            return;

        auto* data = buffer.getWritePointer (0);
        juce::FloatVectorOperations::fill (data + numWritten, data[numWritten - 1], total - numWritten);
    }

    juce::AudioBuffer<float> buffer;

    JUCE_DECLARE_NON_COPYABLE (ControlBuffer)
};

} // namespace cgo
