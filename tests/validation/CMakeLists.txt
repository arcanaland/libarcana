# SPDX-FileCopyrightText: 2026 Adam Fidel
# SPDX-License-Identifier: MIT

# These drive the public validate() over a fixture deck, so they link the
# assembled library rather than a check target. Nothing unit-tests a single
# check against a synthesized check_context yet.
arcana_add_test(ansi_test
    LIBS arcana::arcana
    DEFINES FIXTURES_DIR="${arcana_fixtures_dir}"
    INCLUDES ${CMAKE_CURRENT_SOURCE_DIR})

arcana_add_test(backs_test
    LIBS arcana::arcana
    DEFINES FIXTURES_DIR="${arcana_fixtures_dir}"
    INCLUDES ${CMAKE_CURRENT_SOURCE_DIR})

arcana_add_test(ids_test
    LIBS arcana::arcana
    DEFINES FIXTURES_DIR="${arcana_fixtures_dir}"
    INCLUDES ${CMAKE_CURRENT_SOURCE_DIR})

arcana_add_test(images_test
    LIBS arcana::arcana
    DEFINES FIXTURES_DIR="${arcana_fixtures_dir}"
    INCLUDES ${CMAKE_CURRENT_SOURCE_DIR})
