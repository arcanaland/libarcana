# SPDX-FileCopyrightText: 2026 Adam Fidel
# SPDX-License-Identifier: MIT

set(arcana_checks_base ${PROJECT_SOURCE_DIR}/src/validation)

# One target per check group.
foreach(check IN ITEMS ansi backs cards deck ids images names surrogate)
    arcana_add_internal_library(
        check-${check}
        BASE_DIRS ${arcana_checks_base}
        HEADERS ${check}.hpp
        SOURCES ${check}.cpp
    )

    # PUBLIC: every check header is stated in terms of <context.hpp>.
    arcana_link_internal(arcana-check-${check} PUBLIC arcana-validation-core)
endforeach()

arcana_link_internal(arcana-check-ansi PRIVATE arcana-vocab)
arcana_link_internal(arcana-check-backs PRIVATE arcana-identifiers)
arcana_link_internal(arcana-check-ids PRIVATE arcana-identifiers)
arcana_link_internal(arcana-check-images PRIVATE arcana-vocab)
