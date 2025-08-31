namespace cgo
{

namespace LfoTable
{
    enum Shape
    {
        triangle = 0,
        sine,
        rampUp,
        rampDown,
        numShapes
    };

    template <int numPoints>
    const auto& getSplineTable (Shape shape)
    {
        jassert (shape < numShapes);

        static const auto tables = []
        {
            std::array<Spline<float>, numShapes> result;

            std::vector<float> x (numPoints);
            std::vector<float> y (numPoints);

            for (int i = 0; i < numPoints; i++)
                x[i] = static_cast<float> (i) / (numPoints - 1);

            for (int i = 0; i < numPoints; i++)
                y[i] = x[i] > 0.5f ? 2.0f * (1.0f - x[i]) : 2.0f * x[i];
            result[triangle] = Spline<float> (x, y);

            for (int i = 0; i < numPoints; i++)
                y[i] = (std::sin (juce::MathConstants<float>::twoPi * x[i]) + 1.0f) / 2.0f;
            result[sine] = Spline<float> (x, y);

            for (int i = 0; i < numPoints; i++)
                y[i] = x[i];
            result[rampUp] = Spline<float> (x, y);

            for (int i = 0; i < numPoints; i++)
                y[i] = 1.0f - x[i];
            result[rampDown] = Spline<float> (x, y);

            return result;
        }();

        return tables[shape];
    }

    template <int numPoints>
    float getSpline (Shape shape, float phase)
    {
        return getSplineTable<numPoints> (shape).interpolate (phase);
    }

    template <int numPoints>
    void initSpline()
    {
        juce::ignoreUnused (getSpline<numPoints> (triangle, 0.0f));
    }
} // namespace LfoTable

} // namespace cgo
