# SPDX-FileCopyrightText: 2026 Adam Fidel
# SPDX-License-Identifier: MIT

# Where the test corpora come from: the checked-in fixtures, the reference
# decks, and the specification text the anchor gate reads.
#
# Each downloaded corpus is pinned to a commit and can be pointed at a local
# checkout instead. Everything a test needs to find them ends up in
# `arcana_corpus_defines`, which arcana_add_test hands to every test.

include(FetchContent)

# arcana_declare_corpus(<name> REPO <url> TAG <commit> OUT_DIR <var>
#                       [LOCAL <dir>] [ENABLED <bool>])
#
# Sets <var> in the caller's scope to a directory holding the corpus: LOCAL if
# given, the fetched tree if ENABLED, and empty otherwise. A test handed an
# empty path is expected to skip.
function(arcana_declare_corpus name)
    cmake_parse_arguments(ARG "" "REPO;TAG;OUT_DIR;LOCAL;ENABLED" "" ${ARGN})

    if(ARG_LOCAL)
        set(${ARG_OUT_DIR} ${ARG_LOCAL} PARENT_SCOPE)
        return()
    endif()

    if(NOT ARG_ENABLED)
        set(${ARG_OUT_DIR} "" PARENT_SCOPE)
        return()
    endif()

    FetchContent_Declare(${name}
        GIT_REPOSITORY ${ARG_REPO}
        GIT_TAG ${ARG_TAG}
        # A shallow clone cannot resolve a bare commit, and every corpus here
        # is pinned to one rather than to a branch or tag.
        GIT_SHALLOW FALSE
        # None of these are CMake projects. This stops MakeAvailable looking
        # for a CMakeLists.txt to add.
        SOURCE_SUBDIR no-cmake-here)
    FetchContent_MakeAvailable(${name})

    string(TOLOWER ${name} lower)
    set(${ARG_OUT_DIR} ${${lower}_SOURCE_DIR} PARENT_SCOPE)
endfunction()

set(arcana_fixtures_dir ${CMAKE_CURRENT_SOURCE_DIR}/fixtures)
set(arcana_support_dir ${CMAKE_CURRENT_SOURCE_DIR}/support)

option(ARCANA_FETCH_REFERENCE_DECKS "Download the reference decks used by deck_test" ON)

set(ARCANA_REFERENCE_DECKS_TAG
    6ee23f5ddb7cec005bf7d741a1929adcdd9c1477 CACHE STRING
    "Commit of arcanaland/reference-decks to test against")

set(ARCANA_REFERENCE_DECKS_DIR "" CACHE PATH
    "Use an local reference-decks checkout instead of downloading one")

arcana_declare_corpus(reference_decks
    REPO https://github.com/arcanaland/reference-decks.git
    TAG ${ARCANA_REFERENCE_DECKS_TAG}
    LOCAL ${ARCANA_REFERENCE_DECKS_DIR}
    ENABLED ${ARCANA_FETCH_REFERENCE_DECKS}
    OUT_DIR arcana_reference_decks_dir)

option(ARCANA_FETCH_SPECIFICATION "Download the specification text" ON)

# One commit per schema major, matching src/validation/spec_pin.hpp. The two
# are checked against each other by spec_anchor_test rather than trusted to
# stay in step: the catalogue's citations were read against the header's pin,
# and the anchor gate reads whatever is fetched here.
set(ARCANA_SPECIFICATION_V1_TAG
    29f2184b8fc29e1db016c1f4a3d0c96bac8a4217 CACHE STRING
    "Commit of arcanaland/specifications the major-1 citations are derived from")

set(ARCANA_SPECIFICATION_V2_TAG
    f32d330cdcd84190a80e357ec7b826c4befe5446 CACHE STRING
    "Commit of arcanaland/specifications the major-2 citations are derived from")

set(ARCANA_SPECIFICATION_DIR "" CACHE PATH
    "Read specs from this directory instead of downloading them.")

arcana_declare_corpus(specification_v1
    REPO https://github.com/arcanaland/specifications.git
    TAG ${ARCANA_SPECIFICATION_V1_TAG}
    LOCAL ${ARCANA_SPECIFICATION_DIR}
    ENABLED ${ARCANA_FETCH_SPECIFICATION}
    OUT_DIR arcana_specification_v1_dir)

arcana_declare_corpus(specification_v2
    REPO https://github.com/arcanaland/specifications.git
    TAG ${ARCANA_SPECIFICATION_V2_TAG}
    LOCAL ${ARCANA_SPECIFICATION_DIR}
    ENABLED ${ARCANA_FETCH_SPECIFICATION}
    OUT_DIR arcana_specification_v2_dir)

# The v1.0 text predates the README.md -> DECK.md rename at e7da516, so the two
# majors are not the same filename.
set(arcana_specification_v1_file "")
if(arcana_specification_v1_dir)
    set(arcana_specification_v1_file ${arcana_specification_v1_dir}/README.md)
endif()

set(arcana_specification_v2_file "")
if(arcana_specification_v2_dir)
    set(arcana_specification_v2_file ${arcana_specification_v2_dir}/DECK.md)
endif()

set(arcana_corpus_defines
    FIXTURES_DIR="${arcana_fixtures_dir}"
    REFERENCE_DECKS_DIR="${arcana_reference_decks_dir}"
    SPECIFICATION_V1_FILE="${arcana_specification_v1_file}"
    SPECIFICATION_V2_FILE="${arcana_specification_v2_file}"
    SPECIFICATION_V1_TAG="${ARCANA_SPECIFICATION_V1_TAG}"
    SPECIFICATION_V2_TAG="${ARCANA_SPECIFICATION_V2_TAG}")
