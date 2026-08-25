// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

// Find a card and its file on disk
//
//   ./card_assets ~/decks rider-waite-smith

#include <arcana/library.hpp>

#include <optional>
#include <print>
#include <string>

namespace
{

void show(arcana::deck const& deck, arcana::card_id const& id)
{
    // nullopt when the deck excludes the card or never defined it
    std::optional<arcana::card> card = deck.find_card(id);
    if (!card)
    {
        std::println(
            "{}: not in this deck ({})", id.to_canonical(),
            deck.exclusion_reason(id.to_canonical()).value_or("not declared")
        );
        return;
    }

    std::println("{}: {}", card->canonical_id(), card->display_name);
    if (card->alt_text)
        std::println("  alt text: {}", *card->alt_text);

    // Every image for this card across every image root dir
    for (arcana::card_image const& image : card->images)
    {
        std::string_view const kind = image.kind == arcana::image_kind::scalable ? "scalable"
                                      : image.kind == arcana::image_kind::raster ? "raster"
                                                                                 : "ansi";
        std::println("  {:8} {:8} {}", kind, image.source_dir, image.path.string());
    }

    if (std::optional<arcana::card_image> raster = card->best_raster_for_height(600))
        std::println("  for a 600px slot: {} ({}px tall)", raster->path.string(), *raster->height);

    if (std::optional<arcana::card_image> ansi = card->best_ansi_for_lines(40))
        std::println("  for a 40-line terminal: {} ({} lines)", ansi->path.string(), *ansi->lines);

    if (std::optional<arcana::card_image> svg = card->scalable_image())
        std::println("  scalable: {}", svg->path.string());
}

}  // namespace

int main(int argc, char** argv)
{
    arcana::library_options options;
    if (argc > 1)
        options.roots.emplace_back(argv[1]);

    arcana::deck_library library{options};

    if (library.decks().empty())
    {
        std::println("no decks found");
        return 1;
    }

    std::string const directory_name = argc > 2 ? argv[2] : library.decks().front().directory_name;

    auto loaded = library.load(directory_name);
    if (!loaded)
    {
        std::println("could not load {}: {}", directory_name, loaded.error().message);
        return 1;
    }

    arcana::deck const& deck = **loaded;

    // The Fool
    show(deck, arcana::card_id::standard_major(0));

    // Ace of Cups
    show(deck, arcana::card_id::standard_minor(arcana::suit::cups, arcana::rank::ace));

    // King of Swords
    auto parsed = arcana::card_id::parse("minor_arcana.swords.king");
    if (parsed)
        show(deck, *parsed);

    // Card backs are deck-wide
    if (std::optional<arcana::card_back_design> back = deck.default_card_back_design())
        std::println("default card back: {} {}", back->id, back->image.string());

    if (std::optional<arcana::card> drawn = deck.random_card(42))
        std::println("card for seed 42: {}", drawn->display_name);

    return 0;
}
