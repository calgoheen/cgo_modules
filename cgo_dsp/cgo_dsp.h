/*******************************************************************************
 BEGIN_JUCE_MODULE_DECLARATION

  ID:                 cgo_dsp
  vendor:             cal
  version:            1.0.0
  name:               cgo_dsp
  description:        What you need, when you need it.
  website:            https://www.calgoheen.com
  license:            GPLv3
  minimumCppStandard: 20

  dependencies:       cgo_core, juce_audio_formats, juce_dsp, chowdsp_filters, chowdsp_dsp_utils, chowdsp_waveshapers

 END_JUCE_MODULE_DECLARATION
*******************************************************************************/

#pragma once

#include <cgo_core/cgo_core.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_dsp/juce_dsp.h>
#include <chowdsp_filters/chowdsp_filters.h>
#include <chowdsp_dsp_utils/chowdsp_dsp_utils.h>
#include <chowdsp_waveshapers/chowdsp_waveshapers.h>

JUCE_BEGIN_IGNORE_WARNINGS_GCC_LIKE ("-Wextra-semi", "-Wsign-conversion", "-Wfloat-equal", "-Wgnu-anonymous-struct", "-Wnested-anon-types")
#include "third_party/r8b/CDSPResampler.h"
JUCE_END_IGNORE_WARNINGS_GCC_LIKE

#include "utilities/Phasor.h"
#include "utilities/Curve.h"
#include "utilities/CircularIterator.h"
#include "utilities/SmoothingFilter.h"
#include "utilities/Fifo.h"
#include "utilities/BufferUtils.h"
#include "utilities/LfoShape.h"
#include "utilities/AudioUtils.h"

#include "effects/Chorus.h"
#include "effects/Compressor.h"
#include "effects/Crush.h"
#include "effects/Delay.h"
#include "effects/Distortion.h"
#include "effects/Filter.h"
#include "effects/Flanger.h"
#include "effects/Gate.h"
#include "effects/Phaser.h"
#include "effects/DiffusionReverb.h"
#include "effects/TapeStop.h"
#include "effects/Utility.h"
