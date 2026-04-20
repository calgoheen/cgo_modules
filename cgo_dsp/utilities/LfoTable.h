namespace cgo
{

class LfoTable
{
public:
    enum Shape
    {
        triangle = 0,
        sine,
        rampUp,
        rampDown,
        numShapes
    };

    template <int numPoints>
    static const auto& getSplineTable (Shape shape)
    {
        jassert (shape < numShapes);

        static const auto tables = []
        {
            std::array<Spline<float>, numShapes> result;

            std::vector<float> x (numPoints);
            std::vector<float> y (numPoints);

            for (int i = 0; i < numPoints; i++)
                x[i] = static_cast<float> (i) / (numPoints - 1);

            for (int s = 0; s < numShapes; s++)
            {
                for (int i = 0; i < numPoints; i++)
                    y[i] = evalShape ((Shape) s, x[i]);
                result[s] = Spline<float> (x, y);
            }

            return result;
        }();

        return tables[shape];
    }

    template <int numPoints>
    static float getSpline (Shape shape, float phase)
    {
        return getSplineTable<numPoints> (shape).interpolate (phase);
    }

    template <int numPoints>
    static void initSpline()
    {
        juce::ignoreUnused (getSpline<numPoints> (triangle, 0.0f));
    }

    template <int numPoints>
    static const auto& getLinearTable (Shape shape)
    {
        jassert (shape < numShapes);

        static std::array<juce::dsp::LookupTableTransform<float>, numShapes> tables;
        static std::once_flag initFlag;
        std::call_once (initFlag, [&]
        {
            for (int s = 0; s < numShapes; s++)
                tables[s].initialise ([s] (float x) { return evalShape ((Shape) s, x); },
                                      0.0f, 1.0f, numPoints);
        });

        return tables[shape];
    }

    template <int numPoints>
    static float getLinear (Shape shape, float phase)
    {
        return getLinearTable<numPoints> (shape) (phase);
    }

    template <int numPoints>
    static void initLinear()
    {
        juce::ignoreUnused (getLinear<numPoints> (triangle, 0.0f));
    }

private:
    LfoTable() = delete;

    static float evalShape (Shape shape, float x)
    {
        switch (shape)
        {
            case triangle: return x > 0.5f ? 2.0f * (1.0f - x) : 2.0f * x;
            case sine:     return (std::sin (juce::MathConstants<float>::twoPi * x) + 1.0f) / 2.0f;
            case rampUp:   return x;
            case rampDown: return 1.0f - x;
            default:       jassertfalse; return 0.0f;
        }
    }
};

} // namespace cgo
