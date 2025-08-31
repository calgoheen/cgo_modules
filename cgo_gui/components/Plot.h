namespace cgo
{

class Plot : public juce::Component
{
public:
    Plot (std::vector<float> xValues, std::vector<float> yValues);
    ~Plot() override = default;

    void paint (juce::Graphics& g) override;
    void resized() override;

    template <typename Callable>
    static void plotFunction (Callable&& functionToPlot, float xMin, float xMax, int numPoints)
    {
        jassert (numPoints >= 2);

        const float interval = (xMax - xMin) / static_cast<float> (numPoints - 1);

        std::vector<float> x (numPoints);
        std::vector<float> y (numPoints);

        for (int i = 0; i < numPoints; i++)
        {
            x[i] = xMin + i * interval;
            y[i] = functionToPlot (x[i]);
        }

        juce::DialogWindow::LaunchOptions windowOptions;
        auto plot = new Plot (std::move (x), std::move (y));
        plot->setSize (400, 300);

        windowOptions.content.setOwned (plot);
        windowOptions.resizable = true;
        windowOptions.useNativeTitleBar = true;
        windowOptions.dialogTitle = "Plot";

        windowOptions.launchAsync();
    }

private:
    const int numPoints;
    const std::vector<float> x;
    const std::vector<float> y;
    const juce::Range<float> xRange;
    const juce::Range<float> yRange;

    juce::Path path;
    juce::AffineTransform transform;
};

} // namespace cgo
