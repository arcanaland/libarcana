# SPDX-FileCopyrightText: 2026 Adam Fidel
# SPDX-License-Identifier: MIT

# One target per check group. Each links the engine and nothing else in the
# project except the vocabularies it actually scans with, so a check that starts
# reaching for a sibling group's helper fails to link rather than compiling.
#
# BASE_DIR is the validation directory rather than this one: the headers are
# spelled <checks/ids.hpp>, which keeps `names.hpp` and `deck.hpp` from
# colliding with the loader's headers of the same name.
set(arcana_checks_base ${PROJECT_SOURCE_DIR}/src/validation)

foreach(check IN ITEMS ansi backs cards deck ids images names surrogate)
    arcana_add_internal_library(
        check-${check}
        BASE_DIR ${arcana_checks_base}
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
