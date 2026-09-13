# Treats each target's interface includes as system includes to ignore warnings.
# Accepts target names or paths whose final component is a target name.
function(cgo_ignore_warnings)
    foreach(arg IN LISTS ARGN)
        get_filename_component(target "${arg}" NAME)

        if(NOT TARGET ${target})
            continue()
        endif()

        get_target_property(includes ${target} INTERFACE_INCLUDE_DIRECTORIES)

        if(includes)
            set_target_properties(${target} PROPERTIES
                INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${includes}")
        endif()
    endforeach()
endfunction()
