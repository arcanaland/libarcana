# SPDX-FileCopyrightText: 2026 Adam Fidel
# SPDX-License-Identifier: MIT

find_package(Catch2 3 REQUIRED)
include(Catch)

function(arcana_add_test name)
    cmake_parse_arguments(ARG "" "" "SOURCES;LIBS" ${ARGN})

    add_executable(${name} ${ARG_SOURCES})
    target_link_libraries(${name} PRIVATE Catch2::Catch2WithMain ${ARG_LIBS})

    catch_discover_tests(${name})
endfunction()

arcana_add_test(version_test SOURCES version_test.cpp LIBS arcana::arcana)
arcana_add_test(paths_test SOURCES paths_test.cpp LIBS arcana::arcana)
arcana_add_test(card_test SOURCES card_test.cpp LIBS arcana::arcana)
arcana_add_test(deck_test SOURCES deck_test.cpp LIBS arcana::arcana)
arcana_add_test(library_test SOURCES library_test.cpp LIBS arcana::arcana)

target_compile_definitions(deck_test PRIVATE
    FIXTURES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/fixtures")
target_compile_definitions(library_test PRIVATE
    FIXTURES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/fixtures")

# Real decks to parse, as opposed to the hand-written fixtures above. They're a separate
# repo -- too large to vendor, and aquatic-tarot is CC BY-NC-SA 3.0, so it must never end
# up in an install or a package. FetchContent keeps them in the build tree, which
# satisfies both, and pins the commit so a deck-side change can't turn this repo red on
# its own. Bump ARCANA_REFERENCE_DECKS_TAG deliberately.
option(ARCANA_FETCH_REFERENCE_DECKS "Download the reference decks used by deck_test" ON)
set(ARCANA_REFERENCE_DECKS_TAG 6ee23f5ddb7cec005bf7d741a1929adcdd9c1477 CACHE STRING
    "Commit of arcanaland/reference-decks to test against")
set(ARCANA_REFERENCE_DECKS_DIR "" CACHE PATH
    "Use an existing reference-decks checkout instead of downloading one")

if(ARCANA_REFERENCE_DECKS_DIR)
    set(arcana_reference_decks_dir ${ARCANA_REFERENCE_DECKS_DIR})
elseif(ARCANA_FETCH_REFERENCE_DECKS)
    include(FetchContent)
    FetchContent_Declare(reference_decks
        GIT_REPOSITORY https://github.com/arcanaland/reference-decks.git
        # A full SHA rather than a tarball URL_HASH: git objects are content-addressed,
        # so this pins just as tightly without depending on GitHub's archive bytes
        # staying stable. Shallow is incompatible with fetching an arbitrary commit.
        GIT_TAG ${ARCANA_REFERENCE_DECKS_TAG}
        GIT_SHALLOW FALSE
        # Decks are data, not a project -- point SOURCE_SUBDIR at a directory with no
        # CMakeLists.txt so MakeAvailable populates without trying add_subdirectory.
        SOURCE_SUBDIR no-cmake-here)
    FetchContent_MakeAvailable(reference_decks)
    set(arcana_reference_decks_dir ${reference_decks_SOURCE_DIR})
else()
    # Explicitly opted out, so deck_test's reference-deck cases SKIP.
    set(arcana_reference_decks_dir "")
endif()

target_compile_definitions(deck_test PRIVATE
    REFERENCE_DECKS_DIR="${arcana_reference_decks_dir}")

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
