# SPDX-FileCopyrightText: 2026 Adam Fidel
# SPDX-License-Identifier: MIT

# Resolves a directory of real decks for the tests and the examples to run
# against, and leaves it in `arcana_reference_decks_dir`. Empty when no decks
# are available, which every consumer of it must tolerate.

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
