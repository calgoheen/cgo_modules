#include <cgo_graph/cgo_graph.h>

namespace cgo
{

class ParameterManager::GenericParameter : public juce::RangedAudioParameter, private juce::AudioProcessorParameter::Listener
{
public:
    explicit GenericParameter (int idx) : juce::RangedAudioParameter (juce::ParameterID { "p" + juce::String (idx), 1 }, "Param " + juce::String (idx)) {}

    ~GenericParameter() override { unlink(); }

    void link (juce::RangedAudioParameter& target)
    {
        unlink();

        linked.store (&target, std::memory_order_release);
        target.addListener (this);

        sendValueChangedMessageToListeners (target.getValue());
    }

    void unlink()
    {
        if (auto* current = linked.exchange (nullptr, std::memory_order_acq_rel))
            current->removeListener (this);
    }

    float getValue() const override
    {
        auto* l = linked.load (std::memory_order_acquire);
        return l != nullptr ? l->getValue() : fallbackValue.load();
    }
    float getDefaultValue() const override
    {
        auto* l = linked.load (std::memory_order_acquire);
        return l != nullptr ? l->getDefaultValue() : 0.0f;
    }
    int getNumSteps() const override
    {
        auto* l = linked.load (std::memory_order_acquire);
        return l != nullptr ? l->getNumSteps() : juce::AudioProcessor::getDefaultNumParameterSteps();
    }
    bool isDiscrete() const override
    {
        auto* l = linked.load (std::memory_order_acquire);
        return l != nullptr ? l->isDiscrete() : false;
    }
    bool isBoolean() const override
    {
        auto* l = linked.load (std::memory_order_acquire);
        return l != nullptr ? l->isBoolean() : false;
    }
    juce::String getText (float v, int len) const override
    {
        auto* l = linked.load (std::memory_order_acquire);
        return l != nullptr ? l->getText (v, len) : juce::String (v, 4);
    }
    float getValueForText (const juce::String& text) const override
    {
        auto* l = linked.load (std::memory_order_acquire);
        return l != nullptr ? l->getValueForText (text) : text.getFloatValue();
    }
    juce::String getName (int maxLen) const override
    {
        auto* l = linked.load (std::memory_order_acquire);
        return l != nullptr ? l->getName (maxLen) : juce::RangedAudioParameter::getName (maxLen);
    }
    juce::String getLabel() const override
    {
        auto* l = linked.load (std::memory_order_acquire);
        return l != nullptr ? l->getLabel() : juce::String();
    }

    const juce::NormalisableRange<float>& getNormalisableRange() const override
    {
        auto* l = linked.load (std::memory_order_acquire);
        return l != nullptr ? l->getNormalisableRange() : defaultRange;
    }

    void setValue (float newValue) override
    {
        if (auto* l = linked.load (std::memory_order_acquire))
            l->setValueNotifyingHost (newValue);
        else
            fallbackValue.store (newValue);
    }

private:
    void parameterValueChanged (int, float newValue) override { sendValueChangedMessageToListeners (newValue); }

    void parameterGestureChanged (int, bool gestureIsStarting) override
    {
        if (gestureIsStarting)
            beginChangeGesture();
        else
            endChangeGesture();
    }

    juce::NormalisableRange<float> defaultRange { 0.0f, 1.0f };
    std::atomic<float> fallbackValue { 0.0f };

    std::atomic<juce::RangedAudioParameter*> linked { nullptr };
};

juce::AudioProcessorValueTreeState::ParameterLayout ParameterManager::buildLayout (int n)
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    for (int i = 0; i < n; i++)
        layout.add (std::make_unique<GenericParameter> (i));

    return layout;
}

namespace routingIds
{

const juce::Identifier ROUTING { "ROUTING" };
const juce::Identifier SLOT { "SLOT" };
const juce::Identifier p { "p" };
const juce::Identifier tgtKind { "tgtKind" };
const juce::Identifier tgt { "tgt" };
const juce::Identifier param { "param" };

} // namespace routingIds

ParameterManager::ParameterManager (juce::AudioProcessor& processor, int n) : apvts (processor, nullptr, "PARAMS", buildLayout (n))
{
    genericParameters.reserve ((size_t) n);

    for (int i = 0; i < n; i++)
        genericParameters.push_back (static_cast<GenericParameter*> (apvts.getParameter ("p" + juce::String (i))));

    reset();
}

ParameterManager::~ParameterManager() = default;

void ParameterManager::attach (ParameterOwner& owner)
{
    auto& used = ownerSlots[&owner];

    for (auto* mp : owner.getModulatedParameters())
    {
        if (freeSlots.empty())
        {
            jassertfalse;
            break;
        }

        const auto slot = freeSlots.back();
        freeSlots.pop_back();

        genericParameters[(size_t) slot]->link (mp->parameter);
        used.push_back (slot);
    }
}

void ParameterManager::detach (const ParameterOwner& owner)
{
    const auto it = ownerSlots.find (&owner);

    if (it == ownerSlots.end())
        return;

    for (auto slot : it->second)
    {
        if (slot == unassignedSlot)
            continue;

        genericParameters[(size_t) slot]->unlink();
        freeSlots.push_back (slot);
    }

    ownerSlots.erase (it);
}

int ParameterManager::getNumFreeSlots() const noexcept { return (int) freeSlots.size(); }

void ParameterManager::reset()
{
    for (auto* generic : genericParameters)
        generic->unlink();

    ownerSlots.clear();
    freeSlots.clear();
    freeSlots.reserve (genericParameters.size());

    for (int i = (int) genericParameters.size() - 1; i >= 0; i--)
        freeSlots.push_back (i);
}

std::vector<int> ParameterManager::getSlots (const ParameterOwner& owner) const
{
    const auto it = ownerSlots.find (&owner);

    return it != ownerSlots.end() ? it->second : std::vector<int> {};
}

void ParameterManager::restoreLink (int slotIndex, ParameterOwner& owner, const juce::String& paramID)
{
    const auto& params = owner.getModulatedParameters();
    const int paramIndex = owner.indexOf (paramID);

    if (! juce::isPositiveAndBelow (slotIndex, (int) genericParameters.size()) || paramIndex < 0)
    {
        jassertfalse;
        return;
    }

    const auto free = std::find (freeSlots.begin(), freeSlots.end(), slotIndex);

    // Two saved routings named the same slot
    if (free == freeSlots.end())
    {
        jassertfalse;
        return;
    }

    freeSlots.erase (free);

    auto& used = ownerSlots[&owner];

    if ((int) used.size() <= paramIndex)
        used.resize ((size_t) paramIndex + 1, unassignedSlot);

    genericParameters[(size_t) slotIndex]->link (params[paramIndex]->parameter);
    used[(size_t) paramIndex] = slotIndex;
}

juce::ValueTree ParameterManager::toValueTree (const ModularGraph& graph) const
{
    auto handleOf = [&graph] (const ParameterOwner* owner) -> std::optional<std::pair<juce::String, juce::uint32>>
    {
        for (const auto& entry : graph.processors().getAll())
            if (static_cast<const ParameterOwner*> (entry.processor) == owner)
                return std::pair { NodeKind::processor, entry.id.uid };

        for (const auto& entry : graph.modulators().getAll())
            if (static_cast<const ParameterOwner*> (entry.modulator) == owner)
                return std::pair { NodeKind::modulator, entry.id.uid };

        return std::nullopt;
    };

    struct Routing
    {
        int slot;
        juce::String kind;
        juce::uint32 node;
        juce::String param;
    };

    std::vector<Routing> routings;

    for (const auto& [owner, slots] : ownerSlots)
    {
        const auto handle = handleOf (owner);

        // An owner that is not a node of this graph
        if (! handle.has_value())
        {
            jassertfalse;
            continue;
        }

        const auto& params = owner->getModulatedParameters();

        for (int i = 0; i < (int) slots.size(); i++)
            if (slots[(size_t) i] != unassignedSlot)
                routings.push_back ({ slots[(size_t) i], handle->first, handle->second, params[i]->parameter.getParameterID() });
    }

    std::sort (routings.begin(), routings.end(), [] (const Routing& a, const Routing& b) { return a.slot < b.slot; });

    juce::ValueTree tree { routingIds::ROUTING };

    for (const auto& routing : routings)
    {
        juce::ValueTree slot { routingIds::SLOT };

        slot.setProperty (routingIds::p, routing.slot, nullptr);
        slot.setProperty (routingIds::tgtKind, routing.kind, nullptr);
        slot.setProperty (routingIds::tgt, (int) routing.node, nullptr);
        slot.setProperty (routingIds::param, routing.param, nullptr);

        tree.appendChild (slot, nullptr);
    }

    return tree;
}

void ParameterManager::restoreFromValueTree (const juce::ValueTree& parent, ModularGraph& graph)
{
    reset();

    for (const auto slot : parent.getChildWithName (routingIds::ROUTING))
    {
        if (! slot.hasType (routingIds::SLOT))
            continue;

        const auto kind = slot[routingIds::tgtKind].toString();
        const auto uid = (juce::uint32) (int) slot[routingIds::tgt];

        auto* owner = [&]() -> ParameterOwner*
        {
            if (kind == NodeKind::processor)
            {
                const ProcessorID id { uid };
                return graph.processors().contains (id) ? &graph.processors().getProcessor (id) : nullptr;
            }

            if (kind == NodeKind::modulator)
            {
                const ModulatorID id { uid };
                return graph.modulators().contains (id) ? &graph.modulators().getModulator (id) : nullptr;
            }

            return nullptr;
        }();

        // The node this slot routed to didn't restore
        if (owner == nullptr)
            continue;

        restoreLink (slot[routingIds::p], *owner, slot[routingIds::param].toString());
    }
}

} // namespace cgo
