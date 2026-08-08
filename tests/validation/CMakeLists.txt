# SPDX-FileCopyrightText: 2026 Adam Fidel
# SPDX-License-Identifier: MIT

# One test per src/validation/checks/<area>.cpp. Layers 2-8 of TASK-016 add a
# row here alongside the area they implement.
arcana_add_test(ansi_test
    LIBS arcana::validation
    DEFINES FIXTURES_DIR="${arcana_fixtures_dir}"
    INCLUDES ${CMAKE_CURRENT_SOURCE_DIR})
