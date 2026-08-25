// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

// Every deck installed on this system.
//
//   ./list_decks                     # the XDG deck library
//   ./list_decks ~/decks             # a library root of your own

#include <arcana/library.hpp>
#include <arcana/paths.hpp>

#include <print>
#include <string_view>
#include <vector>

int main(int argc, char** argv)
{
    // empty root uses $XDG_DATA_HOME/tarot/decks
    arcana::library_options options;
    if (argc > 1)
        options.roots.emplace_back(argv[1]);

    std::println("searching {}", arcana::paths::deck_library_path().string());

    arcana::deck_library library{options};

    for (std::filesystem::path const& root : library.roots())
        std::println("  root {}", root.string());

    // Print all decks
    for (arcana::deck_summary const& deck : library.decks())
    {
        std::println(
            "{:24} {:32} v{:8} {:3} cards", deck.directory_name, deck.name, deck.version,
            deck.card_count
        );
        std::println(
            "{:24} id={} at {}", "", deck.identifier.value_or("(unset)"), deck.path.string()
        );
    }

    // Report broken decks
    for (arcana::malformed_deck const& deck : library.malformed_decks())
        std::println("malformed {}: {}", deck.directory_name, deck.problem.message);

    std::println(
        "{} readable, {} malformed", library.decks().size(), library.malformed_decks().size()
    );

    return 0;
}
