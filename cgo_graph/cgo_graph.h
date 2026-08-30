/*******************************************************************************
 BEGIN_JUCE_MODULE_DECLARATION

  ID:                 cgo_graph
  vendor:             cal
  version:            1.0.0
  name:               cgo_graph
  description:        What you need, when you need it.
  website:            https://www.calgoheen.com
  license:            GPLv3
  minimumCppStandard: 20

  dependencies:       cgo_core, cgo_dsp, cgo_plugin

 END_JUCE_MODULE_DECLARATION
*******************************************************************************/

#pragma once

#include <cgo_core/cgo_core.h>
#include <cgo_dsp/cgo_dsp.h>
#include <cgo_plugin/cgo_plugin.h>

#include "utilities/AudioThreadExchange.h"
#include "utilities/ControlBuffer.h"

#include "parameters/Modulation.h"
#include "parameters/ModulatedValue.h"
#include "parameters/ModulatedParameter.h"

#include "base/Identifiers.h"
#include "base/BasicAudioProcessor.h"
#include "base/PlayHeadState.h"
#include "base/ParameterOwner.h"
#include "base/Modulator.h"
#include "base/Processor.h"

#include "graphs/ProcessorGraph.h"
#include "graphs/ModulatorRegistry.h"
#include "graphs/ModulationGraph.h"
#include "graphs/ModularGraph.h"

#include "modulators/Lfo.h"
#include "modulators/Random.h"
#include "modulators/Macro.h"
#include "modulators/EnvelopeFollower.h"

#include "effects/TapeStop.h"
#include "effects/Flanger.h"
#include "effects/Chorus.h"
#include "effects/Phaser.h"
#include "effects/Reverb.h"
#include "effects/Filter.h"
#include "effects/Crush.h"
#include "effects/Delay.h"
#include "effects/Compressor.h"
#include "effects/Gate.h"
#include "effects/Distortion.h"
#include "effects/Utility.h"

#include "session/NodeFactory.h"
#include "session/ParameterManager.h"
