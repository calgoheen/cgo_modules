/*******************************************************************************
 BEGIN_JUCE_MODULE_DECLARATION

  ID:                 cgo_plugin
  vendor:             cal
  version:            1.0.0
  name:               cgo_plugin
  description:        What you need, when you need it.
  website:            https://www.calgoheen.com
  license:            GPLv3
  minimumCppStandard: 20

  dependencies:       cgo_core, juce_audio_processors, juce_audio_formats

 END_JUCE_MODULE_DECLARATION
*******************************************************************************/

#pragma once

#include <cgo_core/cgo_core.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_formats/juce_audio_formats.h>

#include "components/ResizableEditor.h"

#include "utilities/SoundFileUtils.h"
#include "utilities/ParamUtils.h"
