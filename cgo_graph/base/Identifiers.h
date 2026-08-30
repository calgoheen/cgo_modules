namespace cgo
{

template <typename Tag>
struct Identifier
{
    juce::uint32 uid = 0;

    Identifier() = default;
    explicit constexpr Identifier (juce::uint32 value) noexcept : uid (value) {}

    constexpr bool operator== (Identifier other) const noexcept { return uid == other.uid; }
    constexpr bool operator!= (Identifier other) const noexcept { return uid != other.uid; }
    constexpr bool operator< (Identifier other) const noexcept { return uid < other.uid; }
};

using ProcessorID = juce::AudioProcessorGraph::NodeID;
using ModulatorID = Identifier<struct ModulatorIDTag>;
using ConnectionID = Identifier<struct ConnectionIDTag>;

using NodeRef = std::variant<ProcessorID, ModulatorID>;

namespace NodeKind
{

inline const juce::String processor { "proc" };
inline const juce::String modulator { "mod" };

} // namespace NodeKind

} // namespace cgo
