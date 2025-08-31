#include <cgo_gui/cgo_gui.h>

namespace cgo
{

Plot::Plot (std::vector<float> xValues, std::vector<float> yValues)
    : numPoints (xValues.size()),
      x (std::move (xValues)),
      y (std::move (yValues)),
      xRange (x[0], x[numPoints - 1]),
      yRange (juce::findMinimum (y.data(), numPoints), juce::findMaximum (y.data(), numPoints))
{
    jassert (x.size() == y.size());

    path.startNewSubPath (x[0], y[0]);

    for (int i = 1; i < numPoints; i++)
    {
        // X values must be in increasing order
        jassert (x[i] - x[i - 1] > 0.0f);

        path.lineTo (x[i], y[i]);
    }
}

void Plot::paint (juce::Graphics& g)
{
    g.setColour (juce::Colours::red);
    g.strokePath (path, juce::PathStrokeType (2.0f), transform);

    auto pathBounds = path.getBoundsTransformed (transform);
}

void Plot::resized()
{
    const auto bounds = getLocalBounds().reduced (10).toFloat();
    const float dataWidth = xRange.getEnd() - xRange.getStart();
    const float dataHeight = yRange.getEnd() - yRange.getStart();

    const float scaleX = bounds.getWidth() / dataWidth;
    const float scaleY = bounds.getHeight() / dataHeight;

    transform = juce::AffineTransform()
                    .translated (-xRange.getStart(), -yRange.getStart())
                    .scaled (scaleX, -scaleY)
                    .translated (bounds.getX(), bounds.getY() + bounds.getHeight());
}

} // namespace cgo
