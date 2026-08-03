# SPDX-FileCopyrightText: 2026 Adam Fidel
# SPDX-License-Identifier: MIT

find_package(Catch2 3 REQUIRED)
include(Catch)

option(ARCANA_FETCH_REFERENCE_DECKS "Download the reference decks used by deck_test" ON)

set(ARCANA_REFERENCE_DECKS_TAG
    6ee23f5ddb7cec005bf7d741a1929adcdd9c1477 CACHE STRING
    "Commit of arcanaland/reference-decks to test against")

set(ARCANA_REFERENCE_DECKS_DIR "" CACHE PATH
    "Use an local reference-decks checkout instead of downloading one")

if(ARCANA_REFERENCE_DECKS_DIR)
    set(arcana_reference_decks_dir ${ARCANA_REFERENCE_DECKS_DIR})
elseif(ARCANA_FETCH_REFERENCE_DECKS)
    include(FetchContent)

    FetchContent_Declare(reference_decks
        GIT_REPOSITORY https://github.com/arcanaland/reference-decks.git
        GIT_TAG ${ARCANA_REFERENCE_DECKS_TAG}
        # shallow is needed for a commit?
        GIT_SHALLOW FALSE
        SOURCE_SUBDIR no-cmake-here)
    FetchContent_MakeAvailable(reference_decks)
    set(arcana_reference_decks_dir ${reference_decks_SOURCE_DIR})
else()
    set(arcana_reference_decks_dir "")
endif()

function(arcana_add_test name)
    cmake_parse_arguments(ARG "" "" "SOURCES;LIBS;DEFINES;INCLUDES" ${ARGN})

    if(NOT ARG_SOURCES)
        set(ARG_SOURCES ${name}.cpp)
    endif()

    add_executable(${name} ${ARG_SOURCES})

    target_link_libraries(${name} PRIVATE Catch2::Catch2WithMain ${ARG_LIBS} arcana::arcana)
    target_compile_definitions(${name} PRIVATE ${ARG_DEFINES})
    target_include_directories(${name} PRIVATE ${ARG_INCLUDES})

    catch_discover_tests(${name})
endfunction()

set(arcana_fixtures_dir ${CMAKE_CURRENT_SOURCE_DIR}/fixtures)

add_subdirectory(loader)

arcana_add_test(version_test)
arcana_add_test(paths_test)
arcana_add_test(card_test)
arcana_add_test(deck_test DEFINES
    FIXTURES_DIR="${arcana_fixtures_dir}"
    REFERENCE_DECKS_DIR="${arcana_reference_decks_dir}")
arcana_add_test(library_test
    DEFINES FIXTURES_DIR="${arcana_fixtures_dir}"
    INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/loader)

# Consumer tests: build against an installed libarcana
if(ARCANA_INSTALL)
    set(arcana_stage_dir ${PROJECT_BINARY_DIR}/stage)
    set(arcana_staged_prefix ${arcana_stage_dir}${CMAKE_INSTALL_PREFIX})

    # Clean old stage dirs
    add_test(NAME stage_clean
             COMMAND ${CMAKE_COMMAND} -E rm -rf ${arcana_stage_dir})
    add_test(NAME stage_install
             COMMAND ${CMAKE_COMMAND} -E env DESTDIR=${arcana_stage_dir}
                     ${CMAKE_COMMAND} --install ${PROJECT_BINARY_DIR})

    set_tests_properties(stage_clean PROPERTIES FIXTURES_SETUP staged)
    set_tests_properties(stage_install PROPERTIES FIXTURES_SETUP staged
                                                  DEPENDS stage_clean)

    function(arcana_add_consumer_test name)
        cmake_parse_arguments(ARG "FORCE_SCRIPT_CONFIG" "" "" ${ARGN})

        add_test(
            NAME ${name}
            COMMAND
                ${CMAKE_CTEST_COMMAND}
                --build-and-test ${CMAKE_CURRENT_SOURCE_DIR}/consumer
                                 ${CMAKE_CURRENT_BINARY_DIR}/${name}
                --build-generator ${CMAKE_GENERATOR}
                --build-makeprogram ${CMAKE_MAKE_PROGRAM}
                --build-options
                    -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
                    -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
                    -DARCANA_PREFIX=${arcana_staged_prefix}
                    -DARCANA_DEP_PREFIX=${PROJECT_BINARY_DIR}/generators
                    -DARCANA_FORCE_SCRIPT_CONFIG=${ARG_FORCE_SCRIPT_CONFIG}
                --test-command ${CMAKE_CTEST_COMMAND})

        set_tests_properties(${name} PROPERTIES FIXTURES_REQUIRED staged)
    endfunction()

    # test both normal CMake install configs and the new CPS hotness
    arcana_add_consumer_test(consume_cps)
    arcana_add_consumer_test(consume_script FORCE_SCRIPT_CONFIG)
endif()
