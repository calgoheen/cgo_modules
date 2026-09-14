/*******************************************************************************
 BEGIN_JUCE_MODULE_DECLARATION

  ID:                 cgo_gui
  vendor:             cal
  version:            1.0.0
  name:               cgo_gui
  description:        What you need, when you need it.
  website:            https://www.calgoheen.com
  license:            GPLv3
  minimumCppStandard: 20

  dependencies:       cgo_core, juce_gui_basics

 END_JUCE_MODULE_DECLARATION
*******************************************************************************/

#pragma once

#include <cgo_core/cgo_core.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "components/DepthSlider.h"
#include "components/FilmStripSlider.h"
#include "components/ImageRotarySlider.h"
#include "components/ImageThumbSlider.h"
#include "components/ImageToggleButton.h"
#include "components/ModKnob.h"

#include "utilities/FrameStepper.h"
