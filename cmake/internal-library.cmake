# SPDX-FileCopyrightText: 2026 Adam Fidel
# SPDX-License-Identifier: MIT

# arcana_add_internal_library(<name> [BASE_DIRS <dir>...] [HEADERS <h>...] [SOURCES <s>...])
#
# Declares arcana-<name> and arcana::<name>
# (either OBJECT or INTERFACE based on if you pass SOURCES or not)
#
# Every BASE_DIRS entry becomes an include root, so a target whose headers are
# rooted in two places (a public one under include/ and an internal one beside
# its sources) names both rather than gaining a second target.
function(arcana_add_internal_library name)
    cmake_parse_arguments(ARG "" "" "BASE_DIRS;HEADERS;SOURCES" ${ARGN})

    if(NOT ARG_BASE_DIRS)
        set(ARG_BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR})
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
            BASE_DIRS ${ARG_BASE_DIRS}
            FILES ${ARG_HEADERS}
    )

    if(ARG_SOURCES)
        target_sources(arcana-${name} PRIVATE ${ARG_SOURCES})
    endif()
endfunction()

# arcana_link_internal(<target> [PUBLIC <lib>...] [PRIVATE <lib>...] [INTERFACE <lib>...])
#
# Links internal libraries keeping them out of the export set. Both scopes need
# the wrap: PRIVATE deps of a non-INTERFACE target are still recorded in
# INTERFACE_LINK_LIBRARIES as $<LINK_ONLY:...>, which would drag them into
# install(EXPORT).
function(arcana_link_internal target)
    cmake_parse_arguments(ARG "" "" "PUBLIC;PRIVATE;INTERFACE" ${ARGN})

    if(ARG_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR
            "arcana_link_internal(${target}): expected a scope keyword before "
            "${ARG_UNPARSED_ARGUMENTS}")
    endif()

    foreach(scope IN ITEMS PUBLIC PRIVATE INTERFACE)
        set(wrapped "")
        foreach(lib IN LISTS ARG_${scope})
            list(APPEND wrapped $<BUILD_INTERFACE:${lib}>)
        endforeach()
        if(wrapped)
            target_link_libraries(${target} ${scope} ${wrapped})
        endif()
    endforeach()
endfunction()
