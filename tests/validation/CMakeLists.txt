# SPDX-FileCopyrightText: 2026 Adam Fidel
# SPDX-License-Identifier: MIT

arcana_add_test(ansi_test
    LIBS arcana::validation
    DEFINES FIXTURES_DIR="${arcana_fixtures_dir}"
    INCLUDES ${CMAKE_CURRENT_SOURCE_DIR})
