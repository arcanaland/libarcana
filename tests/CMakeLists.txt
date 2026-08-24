# SPDX-FileCopyrightText: 2026 Adam Fidel
# SPDX-License-Identifier: MIT

find_package(Catch2 3 REQUIRED)
include(Catch)

include(${CMAKE_CURRENT_SOURCE_DIR}/corpus.cmake)
include(${PROJECT_SOURCE_DIR}/cmake/test-helpers.cmake)

add_subdirectory(data)
add_subdirectory(loader)
add_subdirectory(validation)

arcana_add_test(version_test LIBS arcana::arcana)
arcana_add_test(paths_test LIBS arcana::arcana)
arcana_add_test(card_test LIBS arcana::arcana)
arcana_add_test(deck_test LIBS arcana::arcana)
arcana_add_test(validation_test LIBS arcana::arcana)
arcana_add_test(spec_anchor_test LIBS arcana::arcana)
arcana_add_test(library_test LIBS arcana::arcana
    INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/loader)

# Consumer tests: build against an installed libarcana
if(ARCANA_INSTALL)
    include(${CMAKE_CURRENT_SOURCE_DIR}/install-tests.cmake)
endif()
