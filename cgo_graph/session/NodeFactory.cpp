#include <cgo_graph/cgo_graph.h>

namespace cgo
{

CGO_ANON_NAMESPACE_BEGIN

template <typename Node>
struct Entry
{
    const char* typeId;
    const char* displayName;
    std::unique_ptr<Node> (*create)();
};

using P = Entry<Processor>;
using M = Entry<Modulator>;

constexpr std::array processorTypes { P { "chorus", "Chorus", []() -> std::unique_ptr<Processor> { return std::make_unique<Chorus>(); } },
                                      P { "compressor", "Compressor", []() -> std::unique_ptr<Processor> { return std::make_unique<Compressor>(); } },
                                      P { "crush", "Crush", []() -> std::unique_ptr<Processor> { return std::make_unique<Crush>(); } },
                                      P { "delay", "Delay", []() -> std::unique_ptr<Processor> { return std::make_unique<Delay>(); } },
                                      P { "distortion", "Distortion", []() -> std::unique_ptr<Processor> { return std::make_unique<Distortion>(); } },
                                      P { "filter", "Filter", []() -> std::unique_ptr<Processor> { return std::make_unique<Filter>(); } },
                                      P { "flanger", "Flanger", []() -> std::unique_ptr<Processor> { return std::make_unique<Flanger>(); } },
                                      P { "gate", "Gate", []() -> std::unique_ptr<Processor> { return std::make_unique<Gate>(); } },
                                      P { "phaser", "Phaser", []() -> std::unique_ptr<Processor> { return std::make_unique<Phaser>(); } },
                                      P { "pitch", "Pitch", []() -> std::unique_ptr<Processor> { return std::make_unique<Pitch>(); } },
                                      P { "reverb", "Reverb", []() -> std::unique_ptr<Processor> { return std::make_unique<Reverb>(); } },
                                      P { "tapestop", "Tape Stop", []() -> std::unique_ptr<Processor> { return std::make_unique<TapeStop>(); } },
                                      P { "utility", "Utility", []() -> std::unique_ptr<Processor> { return std::make_unique<Utility>(); } } };

constexpr std::array modulatorTypes { M { "lfo", "LFO", []() -> std::unique_ptr<Modulator> { return std::make_unique<Lfo>(); } },
                                      M { "random", "Random", []() -> std::unique_ptr<Modulator> { return std::make_unique<Random>(); } },
                                      M { "macro", "Macro", []() -> std::unique_ptr<Modulator> { return std::make_unique<Macro>(); } },
                                      M { "follower", "Follower", []() -> std::unique_ptr<Modulator> { return std::make_unique<EnvelopeFollower>(); } } };

template <typename Node, size_t N>
std::vector<NodeFactory::NodeType> listTypes (const std::array<Entry<Node>, N>& table)
{
    std::vector<NodeFactory::NodeType> types;
    types.reserve (table.size());

    for (const auto& entry : table)
        types.push_back ({ entry.typeId, entry.displayName });

    return types;
}

template <typename Node, size_t N>
const Entry<Node>* findEntry (const std::array<Entry<Node>, N>& table, const juce::String& typeId)
{
    for (const auto& entry : table)
        if (typeId == entry.typeId)
            return &entry;

    return nullptr;
}

template <typename Node, size_t N>
std::unique_ptr<Node> create (const std::array<Entry<Node>, N>& table, const juce::String& typeId)
{
    const auto* entry = findEntry (table, typeId);
    return entry != nullptr ? entry->create() : nullptr;
}

CGO_ANON_NAMESPACE_END

std::vector<NodeFactory::NodeType> NodeFactory::getProcessorTypes() { return ANON::listTypes (ANON::processorTypes); }
std::vector<NodeFactory::NodeType> NodeFactory::getModulatorTypes() { return ANON::listTypes (ANON::modulatorTypes); }

juce::String NodeFactory::getDisplayName (const juce::String& typeId)
{
    if (const auto* entry = ANON::findEntry (ANON::processorTypes, typeId))
        return entry->displayName;

    if (const auto* entry = ANON::findEntry (ANON::modulatorTypes, typeId))
        return entry->displayName;

    return typeId;
}

std::unique_ptr<Processor> NodeFactory::createProcessor (const juce::String& typeId) { return ANON::create (ANON::processorTypes, typeId); }
std::unique_ptr<Modulator> NodeFactory::createModulator (const juce::String& typeId) { return ANON::create (ANON::modulatorTypes, typeId); }

} // namespace cgo