namespace cgo
{

class Modulator;
class ModulatedValue;

/** A range of slots within a ModulationSnapshot's flat slot array. */
struct SlotRange
{
    int begin = 0;
    int count = 0;
};

/** A single modulation connection for use by the audio thread. */
struct ModulationSlot
{
    Modulator* source = nullptr;
    std::shared_ptr<ModulatedValue> depth;
    SlotRange depthSlots; // connections modulating `depth`
    bool bipolar = false;
};

/** The slots applying to a single ModulatedValue. */
struct ModulationView
{
    const ModulationSlot* allSlots = nullptr;
    SlotRange range;

    int size() const noexcept { return range.count; }
    bool isEmpty() const noexcept { return range.count == 0; }

    const ModulationSlot& operator[] (int i) const noexcept { return allSlots[range.begin + i]; }
    ModulationView nested (SlotRange nestedRange) const noexcept { return { allSlots, nestedRange }; }
};

/** Every ModulatedValue on one ParameterOwner, indexed in getModulatedParameters()
    order. A default-constructed instance means "this node has no modulation".
*/
struct NodeModulation
{
    const ModulationSlot* allSlots = nullptr;
    const SlotRange* paramRanges = nullptr;
    int numParams = 0;

    ModulationView viewFor (int paramIndex) const noexcept
    {
        if (! juce::isPositiveAndBelow (paramIndex, numParams))
            return {};

        return { allSlots, paramRanges[paramIndex] };
    }
};

} // namespace cgo
