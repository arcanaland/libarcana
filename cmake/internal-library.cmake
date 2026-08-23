# SPDX-FileCopyrightText: 2026 Adam Fidel
# SPDX-License-Identifier: MIT

# arcana_add_internal_library(<name> [BASE_DIR <dir>] [HEADERS <h>...] [SOURCES <s>...])
#
# Declares arcana-<name> and arcana::<name>
# (either OBJECT or INTERFACE based on if you pass SOURCES or not)
function(arcana_add_internal_library name)
    cmake_parse_arguments(ARG "" "BASE_DIR" "HEADERS;SOURCES" ${ARGN})

    if(NOT ARG_BASE_DIR)
        set(ARG_BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
    endif()

    if(ARG_SOURCES)
        set(scope PUBLIC)
        add_library(arcana-${name} OBJECT)

        # PIC is required for linking the archive into a Python extension
        # module. Hidden visibility keeps the internals out of its dynamic
        # symbol table.
        set_target_properties(
            arcana-${name}
            PROPERTIES
                POSITION_INDEPENDENT_CODE ON
                CXX_VISIBILITY_PRESET hidden
                VISIBILITY_INLINES_HIDDEN ON
        )
    else()
        set(scope INTERFACE)
        add_library(arcana-${name} INTERFACE)
    endif()

    add_library(arcana::${name} ALIAS arcana-${name})

    target_compile_features(arcana-${name} ${scope} cxx_std_26)

    target_sources(
        arcana-${name}
        ${scope}
            FILE_SET HEADERS
            BASE_DIRS ${ARG_BASE_DIR}
            FILES ${ARG_HEADERS}
    )

    if(ARG_SOURCES)
        target_sources(arcana-${name} PRIVATE ${ARG_SOURCES})
    endif()
endfunction()

# arcana_link_internal(<target> <PRIVATE|PUBLIC|INTERFACE> <lib>...)
#
# Links internal libraries keeping them out of the export set.
function(arcana_link_internal target scope)
    set(wrapped "")
    foreach(lib IN LISTS ARGN)
        list(APPEND wrapped $<BUILD_INTERFACE:${lib}>)
    endforeach()

    target_link_libraries(${target} ${scope} ${wrapped})
endfunction()
