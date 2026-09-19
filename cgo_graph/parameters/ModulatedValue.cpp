#include <cgo_graph/cgo_graph.h>

namespace cgo
{

void ModulatedValue::PendingValue::ramp (float newValue) { store (newValue, Kind::ramp); }
void ModulatedValue::PendingValue::snap (float newValue) { store (newValue, Kind::snap); }

ModulatedValue::PendingValue::Write ModulatedValue::PendingValue::take()
{
    auto taken = current.load (std::memory_order_relaxed);

    // Consumes the kind but leaves the value, which peek() reports for the lifetime of the write
    while (taken.kind != Kind::none
           && ! current.compare_exchange_weak (taken, Write { Kind::none, taken.value }, std::memory_order_acquire, std::memory_order_relaxed))
    {
    }

    return taken;
}

float ModulatedValue::PendingValue::peek() const { return current.load (std::memory_order_relaxed).value; }

void ModulatedValue::PendingValue::store (float newValue, Kind newKind) { current.store (Write { newKind, newValue }, std::memory_order_release); }

ModulatedValue::ModulatedValue (const juce::NormalisableRange<float>& r, double smoothing, Rate rateIn)
  : range (r), smoothingTimeSeconds (smoothing), smoothed (smoothing > 0.0), rate (rateIn)
{
}

void ModulatedValue::prepare (double sampleRate, int blockSize)
{
    buffer.prepare (blockSize);
    smoother.reset (sampleRate, smoothingTimeSeconds);

    const float normalized = pending.peek();
    smoother.setCurrentAndTargetValue (normalized);

    lastValue = fromNormalized (normalized);
    currentValue.store (lastValue, std::memory_order_relaxed);
    dirty = false;
    touched = false;
    unsettled = true;
}

void ModulatedValue::process (int numSamples, ModulationView view)
{
    if (numSamples <= 0)
        return;

    using Kind = PendingValue::Kind;

    const auto write = pending.take();

    touched = write.kind == Kind::ramp;

    if (write.kind == Kind::snap)
        smoother.setCurrentAndTargetValue (write.value);
    else if (write.kind == Kind::ramp)
        smoother.setTargetValue (write.value);

    const int numSlots = view.size();

    dirty = numSlots > 0 || write.kind != Kind::none || unsettled;

    if (! dirty)
        return;

    for (int s = 0; s < numSlots; s++)
        view[s].depth->process (numSamples, view.nested (view[s].depthSlots));

    const float newValue = rate == Rate::audio ? writeAudioRate (numSamples, view) : writeBlockRate (numSamples, view);
    const bool ramped = smoothed && ! juce::exactlyEqual (newValue, lastValue);

    lastValue = newValue;
    currentValue.store (newValue, std::memory_order_relaxed);

    unsettled = numSlots > 0 || smoother.isSmoothing() || ramped;
}

float ModulatedValue::fromNormalized (float normalized) const { return range.convertFrom0to1 (juce::jlimit (0.0f, 1.0f, normalized)); }

float ModulatedValue::valueAt (float base, ModulationView view, int index) const
{
    for (int s = 0; s < view.size(); s++)
    {
        const auto& slot = view[s];
        const float modValue = slot.source->getBuffer()[index] - (slot.bipolar ? 0.5f : 0.0f);

        base += slot.depth->getBuffer()[index] * modValue;
    }

    return fromNormalized (base);
}

float ModulatedValue::writeAudioRate (int numSamples, ModulationView view)
{
    const auto block = buffer.write (numSamples);

    for (int i = 0; i < numSamples; i++)
        block[i] = valueAt (smoother.getNextValue(), view, i);

    return block[numSamples - 1];
}

float ModulatedValue::writeBlockRate (int numSamples, ModulationView view)
{
    const float value = valueAt (smoother.skip (numSamples), view, numSamples - 1);

    if (smoothed)
        buffer.writeRamp (lastValue, value, numSamples);
    else
        buffer.writeConstant (value);

    return value;
}

bool ModulatedValue::isChanging() const { return dirty; }

void ModulatedValue::markUnsettled() { unsettled = true; }

bool ModulatedValue::wasTouched() const { return touched; }

const float* ModulatedValue::getBuffer() const { return buffer.read(); }

float ModulatedValue::getCurrentValue() const { return currentValue.load (std::memory_order_relaxed); }

float ModulatedValue::getTargetValue() const { return fromNormalized (pending.peek()); }

void ModulatedValue::setValue (float newValue) { setValueNormalized (range.convertTo0to1 (newValue)); }

void ModulatedValue::setValueNormalized (float newValue) { pending.ramp (juce::jlimit (0.0f, 1.0f, newValue)); }

void ModulatedValue::snapTo (float newValue) { snapToNormalized (range.convertTo0to1 (newValue)); }

void ModulatedValue::snapToNormalized (float newValue)
{
    currentValue.store (fromNormalized (newValue), std::memory_order_relaxed);
    pending.snap (juce::jlimit (0.0f, 1.0f, newValue));
}

} // namespace cgo
