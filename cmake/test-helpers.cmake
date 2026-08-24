# SPDX-FileCopyrightText: 2026 Adam Fidel
# SPDX-License-Identifier: MIT

# arcana_add_test(<name> [SOURCES <s>...] [LIBS <l>...] [DEFINES <d>...] [INCLUDES <i>...])
#
# Declares a Catch2 test executable and registers its cases with ctest.
#
# Every test is given the corpus defines and can include from its own
# directory, so neither has to be repeated per test. Reads two variables that
# tests/corpus.cmake sets: `arcana_corpus_defines` and `arcana_support_dir`.
function(arcana_add_test name)
    cmake_parse_arguments(ARG "" "" "SOURCES;LIBS;DEFINES;INCLUDES" ${ARGN})

    if(NOT ARG_SOURCES)
        set(ARG_SOURCES ${name}.cpp)
    endif()

    add_executable(${name} ${ARG_SOURCES})

    target_link_libraries(${name} PRIVATE Catch2::Catch2WithMain ${ARG_LIBS})
    target_compile_definitions(${name} PRIVATE ${arcana_corpus_defines} ${ARG_DEFINES})
    target_include_directories(${name} PRIVATE
        ${arcana_support_dir} ${CMAKE_CURRENT_SOURCE_DIR} ${ARG_INCLUDES})

    # The random "Checking <VAR> env var" lines are coming from Catch2.
    # Annoying but harmless:
    #   Checking XDG_RUNTIME_DIR env var
    #   Checking TMPDIR env var
    #   Checking TMP env var
    #   Checking TEMP env var
    catch_discover_tests(${name})
endfunction()
