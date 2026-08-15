// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

// Load one deck and print what it is made of.
//
//   ./load_deck                                  # the first deck in the XDG library
//   ./load_deck ~/decks rider-waite-smith        # a root and a directory name

#include <arcana/library.hpp>

#include <expected>
#include <memory>
#include <print>
#include <string>

int main(int argc, char** argv)
{
    arcana::library_options options;
    if (argc > 1)
        options.roots.emplace_back(argv[1]);

    // Name files are read in this order, falling back to English.
    options.languages = {"en"};

    arcana::deck_library library{options};

    if (library.decks().empty())
    {
        std::println("no decks found");
        return 1;
    }

    std::string const directory_name = argc > 2 ? argv[2] : library.decks().front().directory_name;

    // A deck is loaded by its directory name, not by [deck].id: ids are not
    // unique across a library, and `find_all_by_id` returns every deck carrying
    // one.
    std::expected<std::shared_ptr<arcana::deck const>, arcana::error> loaded =
        library.load(directory_name);

    if (!loaded)
    {
        // Everything fallible returns std::expected<T, arcana::error>. The
        // error carries a code to branch on and a message to show.
        std::println("could not load {}: {}", directory_name, loaded.error().message);
        return 1;
    }

    // Loads are cached: asking twice hands back the same deck. The shared_ptr
    // keeps it alive even if the library goes away.
    std::shared_ptr<arcana::deck const> deck = *loaded;

    arcana::deck_metadata const& metadata = deck->metadata;
    std::println("{} {}", metadata.name, metadata.version);
    std::println("  id             {}", metadata.id);
    std::println("  schema_version {}", metadata.schema_version);
    std::println("  artist         {}", metadata.artist.value_or("(unset)"));
    std::println("  license        {}", metadata.license.value_or("(unset)"));
    std::println("  aspect_ratio   {}", metadata.aspect_ratio);
    std::println("  root           {}", deck->root_path.string());

    // The 78 standard cards, minus [excluded_cards], plus [custom_cards].
    std::println(
        "{} cards, {} major", deck->cards.size(),
        deck->cards_of_kind(arcana::arcana_kind::major_arcana).size()
    );

    // Standard suits first, then any custom suit this deck declares. A deck may
    // rename a suit or a court rank, so print the display name, never the key.
    for (arcana::suit_info const& s : deck->suits())
        std::println(
            "  suit {:10} {:12} {:2} cards{}", s.key, s.display_name,
            deck->cards_in_suit(s.key).size(), s.excluded ? " (excluded)" : ""
        );

    std::println("  knight is called a {}", deck->display_rank_name(arcana::rank::knight));

    return 0;
}
