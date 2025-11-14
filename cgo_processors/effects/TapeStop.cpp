#include <cgo_processors/cgo_processors.h>

namespace cgo
{

TapeStop::TapeStop (Params& params)
    : parameters (params)
{
    prevSettings.filter = &filters[0];
    currentSettings.filter = &filters[1];

    addParameterUpdaters (*parameters.mode, [this] (float val)
                          {
                              const auto nextMode = static_cast<Mode> (juce::roundToInt (val));
                          
                              if (currentSettings.mode != nextMode)
                                  updateMode (nextMode);
                          },
                          *parameters.autoBypass, [this] (float val)
                          {
                              autoBypass = val > 0.5f;
                          },
                          *parameters.sync, [this] (float val)
                          {
                              syncActive = val > 0.5f;
                              updateLengths();
                          },
                          *parameters.slowdownLength, [this] (float val)
                          {
                              if (! syncActive)
                                  updateLengths();
                          },
                          *parameters.slowdownLengthSync, [this] (float val)
                          {
                              if (syncActive)
                                  updateLengths();
                          },
                          *parameters.slowdownCurve, [this] (float val)
                          {
                              slowdownCurve = juce::jmap (val, -1.0f, 1.0f, -0.5f, 0.5f);
                          },
                          *parameters.slowdownStart, [this] (float val)
                          {
                              slowdownStart = val;
                          },
                          *parameters.slowdownEnd, [this] (float val)
                          {
                              slowdownEnd = val;
                          },
                          *parameters.speedupLength, [this] (float val)
                          {
                              if (! syncActive)
                                  updateLengths();
                          },
                          *parameters.speedupLengthSync, [this] (float val)
                          {
                              if (syncActive)
                                  updateLengths();
                          },
                          *parameters.speedupCurve, [this] (float val)
                          {
                              speedupCurve = juce::jmap (-val, -1.0f, 1.0f, -0.5f, 0.5f);
                          },
                          *parameters.speedupStart, [this] (float val)
                          {
                              speedupStart = val;
                          },
                          *parameters.speedupEnd, [this] (float val)
                          {
                              speedupEnd = val;
                          },
                          *parameters.fadeLength, [this] (float val)
                          {
                              fadeLengthProportion = val;
                          },
                          *parameters.crossfadeLength, [this] (float val)
                          {
                              crossfadeLengthSamples = juce::roundToInt (val * getSampleRate());
                          },
                          *parameters.filterType, [this] (float val)
                          {
                              const int choice = juce::roundToInt (val);

                              for (auto& f : filters)
                                  f.setMode (choice == 0 ? 0.0f : choice == 1 ? 1.0f : 0.5f);
                          },
                          *parameters.filterCutoff, [this] (float val)
                          {
                              cutoffSmoother.setTargetValue (val);
                          },
                          *parameters.filterResonance, [this] (float val)
                          {
                              resonanceSmoother.setTargetValue (val);
                          });
}

void TapeStop::reset()
{
    cutoffSmoother.reset (getSampleRate(), smoothingTimeSeconds);
    resonanceSmoother.reset (getSampleRate(), smoothingTimeSeconds);

    const int maxDelaySamples = static_cast<int> (maxDelaySeconds * getSampleRate()) + 10;
    delay.emplace (maxDelaySamples);
    delay->prepare (getProcessSpec());

    for (auto& f : filters)
    {
        f.prepare (getProcessSpec());
        f.setCutoffFrequency<false> (cutoffSmoother.getCurrentValue());
        f.setQValue<true> (resonanceSmoother.getCurrentValue());
    }

    crossfadeBuffer.setSize (getNumChannels(), 1);
    crossfadeBuffer.clear();

    updateMode (static_cast<Mode> (juce::roundToInt (ParamUtils::getScaledValue (parameters.mode))));
    currentSettings.counter = currentSettings.length;
}

void TapeStop::process (float* const* buffer, int startIndex, int numSamples)
{
    const int endIndex = startIndex + numSamples;
    const auto crossfadeBufferPtr = crossfadeBuffer.getArrayOfWritePointers();

    for (int i = startIndex; i < endIndex; i++)
        for (int j = 0; j < getNumChannels(); j++)
            delay->pushSample (j, buffer[j][i]);

    for (int i = startIndex; i < endIndex; i++)
    {
        if (cutoffSmoother.isSmoothing() || resonanceSmoother.isSmoothing())
        {
            const float nextCutoff = cutoffSmoother.getNextValue();
            const float nextResonance = resonanceSmoother.getNextValue();

            for (auto& f : filters)
            {
                f.setCutoffFrequency<false> (nextCutoff);
                f.setQValue<true> (nextResonance);
            }
        }

        for (int j = 0; j < getNumChannels(); j++)
            crossfadeBufferPtr[j][0] = buffer[j][i];
        
        processSample (buffer, i, currentSettings);

        if (currentSettings.counter <= currentSettings.crossfadeLength)
        {
            processSample (crossfadeBufferPtr, 0, prevSettings);

            const float mix = static_cast<float> (currentSettings.counter) / static_cast<float> (currentSettings.crossfadeLength);
            const auto [prevGain, currentGain] = AudioUtils::constantPowerMix (mix);
            
            for (int j = 0; j < getNumChannels(); j++)
                buffer[j][i] = buffer[j][i] * currentGain + crossfadeBufferPtr[j][0] * prevGain;
        }

        for (int j = 0; j < getNumChannels(); j++)
            delay->incrementReadPointer (j);

        if (autoBypass 
            && currentSettings.mode != Mode::bypass
            && currentSettings.length - currentSettings.counter < crossfadeLengthSamples / 2)
        {
            updateMode (Mode::bypass);
        }
    }
}

void TapeStop::tempoChanged()
{
    if (syncActive)
        updateLengths();
}

void TapeStop::processSample (float* const* buffer, int sampleIndex, Settings& settings)
{
    if (settings.mode != Mode::bypass)
    {
        if (settings.counter < settings.length)
        {
            const double position = static_cast<double> (settings.counter + 1) / static_cast<double> (settings.length);
            const double curved = Curve::exponential (position, settings.curve);
            const double mapped = juce::jmap (curved, settings.start, settings.end);

            settings.delay += settings.mode == Mode::stop ? mapped : 1.0 - mapped;
        }

        const float fadeGain = [&settings]
        {
            const int fadePosition = settings.mode == Mode::stop ? (settings.length - settings.counter) : settings.counter;
            
            if (fadePosition < settings.fadeLength)
                return std::pow (static_cast<float> (fadePosition) / static_cast<float> (settings.fadeLength), 2.5f);

            return 1.0f;
        }();

        for (int j = 0; j < getNumChannels(); j++)
        {
            const float delayOut = delay->popSample (j, settings.delay, false);
            const float filterOut = settings.filter->processSample (j, delayOut);
            buffer[j][sampleIndex] = fadeGain * filterOut;
        }
    }

    if (settings.counter < settings.length)
        ++settings.counter;
}

void TapeStop::updateMode (Mode nextMode)
{
    const auto nextFilterToUse = prevSettings.filter;
    prevSettings = currentSettings;

    currentSettings.mode = nextMode;
    currentSettings.counter = 0;
    currentSettings.delay = 0.0;
    
    if (nextMode == Mode::bypass)
    {
        currentSettings.length = crossfadeLengthSamples;
    }
    else if (nextMode == Mode::stop)
    {
        currentSettings.length = slowdownLengthSamples;
        currentSettings.curve = slowdownCurve;
        currentSettings.start = slowdownStart;
        currentSettings.end = slowdownEnd;
    }
    else
    {
        currentSettings.length = speedupLengthSamples;
        currentSettings.curve = speedupCurve;
        currentSettings.start = speedupStart;
        currentSettings.end = speedupEnd;
    }

    currentSettings.fadeLength = juce::jmax (static_cast<int> (fadeLengthProportion * currentSettings.length), 1);
    currentSettings.crossfadeLength = juce::jmin (currentSettings.length, crossfadeLengthSamples);

    nextFilterToUse->reset();
    currentSettings.filter = nextFilterToUse;
}

void TapeStop::updateLengths()
{
    if (syncActive)
    {
        slowdownLengthSamples = [this]
        {
            const int slowdownLengthIndex = juce::roundToInt (ParamUtils::getScaledValue (parameters.slowdownLengthSync));
            const float slowdownLengthBeats = ParamUtils::syncedRateIndexToBeats (slowdownLengthIndex);
            const float slowdownLengthSeconds = juce::jmin (slowdownLengthBeats * static_cast<float> (getSecondsPerBeat()), maxDelaySeconds);
            return juce::roundToInt (slowdownLengthSeconds * getSampleRate());
        }();

        speedupLengthSamples = [this]
        {
            const int speedupLengthIndex = juce::roundToInt (ParamUtils::getScaledValue (parameters.speedupLengthSync));
            const float speedupLengthBeats = ParamUtils::syncedRateIndexToBeats (speedupLengthIndex);
            const float speedupLengthSeconds = juce::jmin (speedupLengthBeats * static_cast<float> (getSecondsPerBeat()), maxDelaySeconds);
            return juce::roundToInt (speedupLengthSeconds * getSampleRate());
        }();
    }
    else
    {
        slowdownLengthSamples = [this]
        {
            const float slowdownLengthSeconds = ParamUtils::getScaledValue (parameters.slowdownLength);
            return juce::roundToInt (slowdownLengthSeconds * getSampleRate());
        }();

        speedupLengthSamples = [this]
        {
            const float speedupLengthSeconds = ParamUtils::getScaledValue (parameters.speedupLength);
            return juce::roundToInt (speedupLengthSeconds * getSampleRate());
        }();
    }
}

} // namespace cgo
