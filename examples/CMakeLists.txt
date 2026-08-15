# SPDX-FileCopyrightText: 2026 Adam Fidel
# SPDX-License-Identifier: MIT

# The programs quoted by docs/. Each one is buildable on its own and runs
# against a library root given on the command line, so the same binary a reader
# runs by hand is the one CI runs against the reference decks.

function(arcana_add_example name)
    cmake_parse_arguments(ARG "" "PASS_REGEX" "" ${ARGN})

    add_executable(${name} ${name}.cpp)
    target_link_libraries(${name} PRIVATE arcana::arcana)

    # Without decks there is nothing for the example to say, so it is built but
    # not run.
    if(arcana_reference_decks_dir)
        add_test(NAME example_${name}
                 COMMAND ${name} ${arcana_reference_decks_dir} rider-waite-smith)
        set_tests_properties(example_${name} PROPERTIES LABELS examples)

        # An example that exits non-zero by design is judged on what it printed.
        if(ARG_PASS_REGEX)
            set_tests_properties(example_${name} PROPERTIES
                                 PASS_REGULAR_EXPRESSION ${ARG_PASS_REGEX})
        endif()
    endif()
endfunction()

arcana_add_example(list_decks)
arcana_add_example(load_deck)
arcana_add_example(card_assets)

# Reports a non-conforming deck with exit 1, and whether the reference decks
# stay clean as rules land is not this gate's business.
arcana_add_example(validate_deck PASS_REGEX "findings, [0-9]+ of them errors")
