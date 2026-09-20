# Reports whether a file can be created in ${path}
function(_cgo_is_writable path out)
    while(NOT IS_DIRECTORY "${path}")
        get_filename_component(parent "${path}" DIRECTORY)

        if(parent STREQUAL path)
            break()
        endif()

        set(path "${parent}")
    endwhile()

    set(probe "${path}/.cgo-write-probe")

    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E touch "${probe}"
        RESULT_VARIABLE failed
        OUTPUT_QUIET
        ERROR_QUIET)

    if(failed)
        set(${out} FALSE PARENT_SCOPE)
    else()
        file(REMOVE "${probe}")
        set(${out} TRUE PARENT_SCOPE)
    endif()
endfunction()

# Copies each built plugin to its install location when CGO_INSTALL_AFTER_BUILD is on
function(cgo_install_plugin_after_build target)
    if(NOT CGO_INSTALL_AFTER_BUILD)
        return()
    endif()

    if(NOT TARGET ${target})
        message(FATAL_ERROR "cgo_install_plugin_after_build: no such target ${target}")
    endif()

    get_target_property(juce_copies ${target} JUCE_COPY_PLUGIN_AFTER_BUILD)

    if(juce_copies)
        message(FATAL_ERROR "COPY_PLUGIN_AFTER_BUILD must be set to FALSE in juce_add_plugin()")
    endif()

    if(APPLE)
        set(dirs
            AU   "/Library/Audio/Plug-Ins/Components" component
            VST3 "/Library/Audio/Plug-Ins/VST3"       vst3
            CLAP "/Library/Audio/Plug-Ins/CLAP"       clap)
        set(own "/Library/Audio/Plug-Ins")
    elseif(WIN32)
        set(dirs
            VST3 "$ENV{CommonProgramW6432}/VST3" vst3
            CLAP "$ENV{CommonProgramW6432}/CLAP" clap)
    else()
        set(dirs
            VST3 "$ENV{HOME}/.vst3" vst3
            CLAP "$ENV{HOME}/.clap" clap)
        set(own "$ENV{HOME}/.vst3 $ENV{HOME}/.clap")
    endif()

    get_target_property(product ${target} JUCE_PRODUCT_NAME)

    while(dirs)
        list(POP_FRONT dirs format destination extension)

        set(plugin ${target}_${format})

        if(NOT TARGET ${plugin})
            continue()
        endif()

        _cgo_is_writable("${destination}" writable)

        set(installed "${destination}/${product}.${extension}")

        if(NOT writable)
            list(APPEND blocked "${destination}")
        elseif(IS_DIRECTORY "${installed}")
            _cgo_is_writable("${installed}" writable)

            if(NOT writable)
                list(APPEND blocked "${installed}")
            endif()
        endif()

        get_target_property(artefact ${plugin} JUCE_PLUGIN_ARTEFACT_FILE)

        if(APPLE)
            add_custom_command(TARGET ${plugin} POST_BUILD
                COMMAND "${CMAKE_COMMAND}"
                    "-Dsrc=$<GENEX_EVAL:${artefact}>"
                    "-P" "${JUCE_CMAKE_UTILS_DIR}/checkBundleSigning.cmake"
                VERBATIM)
        endif()

        add_custom_command(TARGET ${plugin} POST_BUILD
            COMMAND "${CMAKE_COMMAND}"
                "-Dsrc=$<GENEX_EVAL:${artefact}>"
                "-Ddest=${destination}"
                "-P" "${JUCE_CMAKE_UTILS_DIR}/copyDir.cmake"
            VERBATIM)
    endwhile()

    if(blocked)
        list(JOIN blocked "\n    " listing)

        if(own)
            set(remedy
                "Take ownership of the plug-in folders and configure again:\n"
                "    sudo chown -R \"$USER\" ${own}")
            string(JOIN "" remedy ${remedy})
        else()
            set(remedy "Configure and build from a shell running as Administrator.")
        endif()

        message(FATAL_ERROR
            "CGO_INSTALL_AFTER_BUILD is on, but these install locations are "
            "not writable:\n"
            "    ${listing}\n"
            "${remedy}\n")
    endif()
endfunction()
