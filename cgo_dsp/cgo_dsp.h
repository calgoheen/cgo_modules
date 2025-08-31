/*******************************************************************************
 BEGIN_JUCE_MODULE_DECLARATION

  ID:                 cgo_dsp
  vendor:             cal
  version:            1.0.0
  name:               cgo_dsp
  description:        What you need, when you need it.
  website:            https://www.calgoheen.com
  license:            GPLv3
  minimumCppStandard: 17

  dependencies:       juce_audio_processors, juce_audio_formats, juce_dsp

 END_JUCE_MODULE_DECLARATION
*******************************************************************************/

#pragma once

#include <juce_events/juce_events.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_dsp/juce_dsp.h>

JUCE_BEGIN_IGNORE_WARNINGS_GCC_LIKE ("-Wextra-semi", "-Wsign-conversion", "-Wfloat-equal", "-Wgnu-anonymous-struct", "-Wnested-anon-types")
#include "utilities/r8b/CDSPResampler.h"
JUCE_END_IGNORE_WARNINGS_GCC_LIKE

#include "utilities/Spline.h"
#include "utilities/Phasor.h"
#include "utilities/Curve.h"
#include "utilities/CircularIterator.h"
#include "utilities/SmoothingFilter.h"
#include "utilities/SignalSaver.h"
#include "utilities/Fifo.h"
#include "utilities/BufferUtils.h"
#include "utilities/LfoTable.h"
#include "utilities/AudioUtils.h"
