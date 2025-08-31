/*******************************************************************************
 BEGIN_JUCE_MODULE_DECLARATION

  ID:                 cgo_plugin
  vendor:             cal
  version:            1.0.0
  name:               cgo_plugin
  description:        What you need, when you need it.
  website:            https://www.calgoheen.com
  license:            GPLv3
  minimumCppStandard: 17

  dependencies:       juce_audio_processors, juce_audio_formats

 END_JUCE_MODULE_DECLARATION
*******************************************************************************/

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_formats/juce_audio_formats.h>

#include "utilities/Iterators.h"
#include "utilities/OptionalPointer.h"
#include "utilities/SoundFileUtils.h"

#include "state/ParameterListener.h"
#include "state/ParamUtils.h"
#include "state/ParamHolder.h"
