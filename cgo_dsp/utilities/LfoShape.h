namespace cgo::dsp::LfoShape
{

enum Shape
{
    triangle = 0,
    sine,
    rampUp,
    rampDown,
    numShapes
};

void init();

float get (Shape shape, float phase, float freqHz);

} // namespace cgo::dsp::LfoShape
