# SPDX-FileCopyrightText: 2026 Adam Fidel
# SPDX-License-Identifier: MIT

# arcana_add_internal_library(<name> [BASE_DIR <dir>] [HEADERS <h>...] [SOURCES <s>...])
#
# Declares arcana-<name> and arcana::<name>
function(arcana_add_internal_library name)
    cmake_parse_arguments(ARG "" "BASE_DIR" "HEADERS;SOURCES" ${ARGN})

    if(NOT ARG_BASE_DIR)
        set(ARG_BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
    endif()

    add_library(arcana-${name} OBJECT)
    add_library(arcana::${name} ALIAS arcana-${name})

    # PIC is required for the shared build and for linking the archive into a
    # Python extension module.
    set_target_properties(
        arcana-${name}
        PROPERTIES
            POSITION_INDEPENDENT_CODE ON
            CXX_VISIBILITY_PRESET hidden
            VISIBILITY_INLINES_HIDDEN ON
    )

    target_compile_features(arcana-${name} PUBLIC cxx_std_26)

    target_sources(
        arcana-${name}
        PUBLIC
            FILE_SET HEADERS
            BASE_DIRS ${ARG_BASE_DIR}
            FILES ${ARG_HEADERS}
        PRIVATE ${ARG_SOURCES}
    )
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
