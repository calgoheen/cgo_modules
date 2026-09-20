list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(GenerateUnits)
include(IgnoreWarnings)
include(InstallPlugin)

option(CGO_INSTALL_AFTER_BUILD
    "Copy built plugins to their install locations after building" OFF)

juce_add_modules(ALIAS_NAMESPACE cgo
    cgo_core
    cgo_dsp
    cgo_plugin
    cgo_graph
    cgo_gui)

cgo_generate_units(cgo_dsp EXCLUDE third_party)
cgo_generate_units(cgo_plugin)
cgo_generate_units(cgo_graph)
cgo_generate_units(cgo_gui)

# Every module depends on cgo_core, so it carries the path to the lists above.
target_include_directories(cgo_core INTERFACE "${CGO_GENERATED_DIR}")
