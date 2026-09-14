namespace cgo
{

class ModKnob : public juce::Slider, public juce::DragAndDropTarget, private juce::ComponentListener
{
public:
    struct Ring
    {
        float depth = 0.0f;
        bool bipolar = false;
        bool isPreview = false;
        bool depthModulated = false;

        bool operator== (const Ring& other) const;
    };

    enum ColourIds
    {
        troughColourId = 0x1e00100,
        discOutlineColourId,
        valueColourId,
        modulationColourId,
        liveValueColourId
    };

    static constexpr float unfocusedAlpha = 0.6f;

    ModKnob();
    ~ModKnob() override;

    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseUp (const juce::MouseEvent& e) override;
    void mouseEnter (const juce::MouseEvent& e) override;
    void mouseMove (const juce::MouseEvent& e) override;
    void mouseExit (const juce::MouseEvent& e) override;

    bool isInterestedInDragSource (const SourceDetails& details) override;
    void itemDragEnter (const SourceDetails& details) override;
    void itemDragExit (const SourceDetails& details) override;
    void itemDropped (const SourceDetails& details) override;

    void setModulation (std::optional<Ring> ring, bool hasUnshownModulation);
    void setLiveValue (float normalisedValue);
    void setLiveDepth (float depth);
    void setAcceptsDrops (bool shouldAccept);

    std::function<void (float depth)> onDepthChanged;
    std::function<void()> onDepthGestureStart;
    std::function<void()> onDepthGestureEnd;
    std::function<void (const juce::var& payload)> onDrop;
    std::function<void()> onRightClick;

private:
    struct Geometry
    {
        juce::Point<float> centre;
        float modRadius = 0.0f;
        float valueRadius = 0.0f;
        float discRadius = 0.0f;
    };

    void componentMovedOrResized (juce::Component& component, bool wasMoved, bool wasResized) override;
    void componentBeingDeleted (juce::Component& component) override;

    void setHandleHover (bool isOverHandle);
    void liftPopupDisplay();
    Geometry getGeometry() const;
    bool handleIsGrabbable() const;
    juce::Rectangle<float> getHandleBounds() const;
    bool hitsHandle (juce::Point<float> position) const;

    std::optional<Ring> ring;
    bool unshownModulation = false;
    float liveValue = 0.0f;
    float liveDepth = 0.0f;

    bool handleHover = false;
    bool draggingDepth = false;
    float depthAtDragStart = 0.0f;

    bool acceptsDrops = true;
    bool dragOver = false;

    juce::Component* trackedPopup = nullptr;
    std::optional<int> liftedPopupY;

    JUCE_DECLARE_NON_COPYABLE (ModKnob)
};

} // namespace cgo
