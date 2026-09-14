namespace cgo
{

/** Audio thread view of the current modulation state. Compiled on the message thread and published 
    through AudioThreadExchange. Uses juce::ReferenceCountedObjectPtr to keep nodes alive as long 
    as the snapshot exists.
*/
struct ModulationSnapshot
{
    struct ModulatorEntry
    {
        Modulator::Ptr modulator;
        NodeModulation modulation;
    };

    struct ProcessorEntry
    {
        juce::AudioProcessorGraph::Node::Ptr node;
        NodeModulation modulation;
    };

    /** Flat list of all active modulations, grouped by target. If one parameter has multiple 
        modulation sources targeting it, those slots will appear consecutively in this list.
    */
    std::vector<ModulationSlot> slots;

    /** List of modulations for every parameter of every node. Unmodulated parameters are 
        present with an empty range, so the size is always equal to the total number of 
        parameters in each modulator and processor in the snapshot. Grouped by node: modulators 
        first, then processors. Sorted by parameter index within each grouping.
    */
    std::vector<SlotRange> paramRanges;

    std::vector<ModulatorEntry> modulators; /**< All modulators used in the snapshot in the required processing order. */
    std::vector<ProcessorEntry> processors; /**< All processors used in the snapshot. */
};

/** Manages modulation connections in a ModularGraph and compiles the ModulationSnapshot from them.
    This includes determining the modulator process order based on the mod-to-mod connections present.
    Cycles are allowed and are resolved with a one-block delay. Compiles new snapshots on the message
    thread, which are synced with the audio thread via AudioThreadExchange.
    
    Also owns the ValueTree structure used to save / restore modulation state.
*/
class ModulationGraph
{
public:
    /** A ModulatedParameter on a node, by index into its getModulatedParameters(). */
    struct ParamTarget
    {
        NodeRef node;
        int index;

        bool operator== (const ParamTarget& other) const { return node == other.node && index == other.index; }
    };

    /** The depth of an existing connection. */
    struct DepthTarget
    {
        ConnectionID connection;

        bool operator== (const DepthTarget& other) const { return connection == other.connection; }
    };

    using Target = std::variant<ParamTarget, DepthTarget>;

    /** Flattened view of a single connection, for external readers. */
    struct ModulationEntry
    {
        ConnectionID id;
        ModulatorID source;
        Target target;
        float depth;
        bool bipolar;
    };

    /** Holds off snapshot compilation while it is alive, so that a burst of edits publishes
        only once. Nests; the outermost one compiles.
    */
    class ScopedBatch
    {
    public:
        explicit ScopedBatch (ModulationGraph& graph);
        ~ScopedBatch();

    private:
        ModulationGraph& graph;

        JUCE_DECLARE_NON_COPYABLE (ScopedBatch)
    };

    ModulationGraph (const ProcessorGraph& processors, const ModulatorRegistry& modulators);

    /** Creates a new modulation targeting a parameter on any node. Self-modulation and
        duplicate connections are not allowed and return nullopt.
    */
    std::optional<ConnectionID> addModulation (ModulatorID source, NodeRef targetNode, int targetParam, float initialDepth = 1.0f, bool bipolar = false);

    /** Creates a new modulation targeting an existing connection's depth. Self-modulation and 
        duplicate connections are not allowed and return nullopt.
    */
    std::optional<ConnectionID> addDepthModulation (ConnectionID connection, ModulatorID source, float initialDepth = 0.0f, bool bipolar = false);

    /** Removes an existing connection. */
    void removeModulation (ConnectionID id);

    /** Sets the depth value of an existing modulation. Does not force a new snapshot to be compiled. */
    void setModulationDepth (ConnectionID id, float depth);

    /** Sets the polarity of an existing modulation. Forces a new snapshot to be compiled. */
    void setModulationBipolar (ConnectionID id, bool bipolar);

    /** Finds an existing connection from source to a parameter on a node. Returns nullopt
        if there is none.
    */
    std::optional<ConnectionID> findModulation (ModulatorID source, NodeRef targetNode, int targetParam) const;

    /** Finds an existing connection from source to the depth of another connection. Returns
        nullopt if there is none.
    */
    std::optional<ConnectionID> findDepthModulation (ModulatorID source, ConnectionID connection) const;

    /** Returns true if addDepthModulation would succeed. */
    bool canAddDepthModulation (ConnectionID connection, ModulatorID source) const;

    /** Every connection in the graph, ordered by ConnectionID. */
    std::vector<ModulationEntry> getModulations() const;

    /** Every connection targeting a parameter on a node, ordered by ConnectionID. Empty if
        the parameter is unmodulated.
    */
    std::vector<ModulationEntry> getModulationsFor (NodeRef node, int param) const;

    /** Every connection targeting the depth of an existing connection, ordered by ConnectionID. */
    std::vector<ModulationEntry> getDepthModulations (ConnectionID connection) const;

    /** Every connection with the given source, whatever it targets. Ordered by ConnectionID. */
    std::vector<ModulationEntry> getModulationsFrom (ModulatorID source) const;

    /** The connection with this ID, or nullopt if there is none. */
    std::optional<ModulationEntry> getModulation (ConnectionID id) const;

    void restoreModulations (const std::vector<ModulationEntry>& entries);

private:
    friend class ModularGraph;

    struct Connection
    {
        ModulatorID source;
        Target target;
        bool bipolar;
        std::shared_ptr<ModulatedValue> depth;
    };

    void prepare (double sampleRate, int blockSize);
    void compile();

    ModulationSnapshot* loadSnapshot();

    juce::ValueTree toValueTree() const;
    void restoreFromValueTree (const juce::ValueTree& graphTree);

    void mintModulations (const std::vector<ModulationEntry>& entries);

    void removeModulationsReferencing (ProcessorID node);
    void removeModulationsReferencing (ModulatorID node);

    ConnectionID addConnection (ModulatorID source, Target target, float initialDepth, bool bipolar, std::optional<ConnectionID> explicitId);
    std::optional<ConnectionID> mintModulation (ModulatorID source, Target target, float initialDepth, bool bipolar, std::optional<ConnectionID> explicitId);
    std::optional<ConnectionID> addModulation (ModulatorID source, Target target, float initialDepth, bool bipolar, std::optional<ConnectionID> explicitId);
    void removeConnection (ConnectionID id);

    bool contains (const NodeRef& node) const;
    bool canResolve (const Target& target) const;
    std::optional<ConnectionID> findModulation (ModulatorID source, const Target& target) const;
    std::vector<ModulationEntry> getModulationsFor (const Target& target) const;

    std::optional<NodeRef> ownerNodeOf (ConnectionID id) const;
    std::optional<NodeRef> ownerNodeOf (const Target& target) const;

    Modulator& resolveSource (ModulatorID id) const;
    const ParameterOwner& resolveNode (const NodeRef& node) const;

    const ProcessorGraph& processorGraph;
    const ModulatorRegistry& modulatorRegistry;

    juce::uint32 nextId = 0;
    std::map<ConnectionID, Connection> connections;

    AudioThreadExchange<ModulationSnapshot> snapshotExchange;

    int batchDepth = 0;
    bool compilePending = false;

    bool prepared = false;
    double cachedSampleRate = 0.0;
    int cachedBlockSize = 0;

    JUCE_DECLARE_NON_COPYABLE (ModulationGraph)
};

} // namespace cgo
