/*******************************************************************************
 BEGIN_JUCE_MODULE_DECLARATION

  ID:                 cgo_core
  vendor:             cal
  version:            1.0.0
  name:               cgo_core
  description:        What you need, when you need it.
  website:            https://www.calgoheen.com
  license:            GPLv3
  minimumCppStandard: 20

  dependencies:

 END_JUCE_MODULE_DECLARATION
*******************************************************************************/

#pragma once

#define CGO_ANON_NAMESPACE_BEGIN \
    namespace                    \
    {                            \
    namespace ANON               \
    {

#define CGO_ANON_NAMESPACE_END \
    }                          \
    }
