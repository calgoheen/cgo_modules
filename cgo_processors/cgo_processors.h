/*******************************************************************************
 BEGIN_JUCE_MODULE_DECLARATION

  ID:                 cgo_processors
  vendor:             cal
  version:            1.0.0
  name:               cgo_processors
  description:        What you need, when you need it.
  website:            https://www.calgoheen.com
  license:            GPLv3
  minimumCppStandard: 17

  dependencies:       cgo_dsp, cgo_plugin, chowdsp_dsp_utils, chowdsp_filters

 END_JUCE_MODULE_DECLARATION
*******************************************************************************/

#pragma once

#include <cgo_dsp/cgo_dsp.h>
#include <cgo_plugin/cgo_plugin.h>
#include <chowdsp_dsp_utils/chowdsp_dsp_utils.h>
#include <chowdsp_filters/chowdsp_filters.h>

#include "base/ProcessParameterUpdater.h"
#include "base/Processor.h"

#include "effects/Flanger.h"
#include "effects/Chorus.h"
