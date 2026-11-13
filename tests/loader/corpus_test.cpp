// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

// What the reference decks load as, beyond whether they load at all

#include <arcana/card.hpp>
#include <arcana/deck.hpp>
#include <arcana/loader.hpp>

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <filesystem>
#include <string>
#include <string_view>

namespace
{

std::string display_name_of(arcana::deck const& d, std::string_view canonical)
{
    auto const at = std::ranges::find_if(
        d.cards(), [canonical](arcana::card const& c) { return c.canonical_id() == canonical; }
    );

    REQUIRE(at != d.cards().end());
    return at->display_name;
}

arcana::card const& card_of(arcana::deck const& d, std::string_view canonical)
{
    auto const at = std::ranges::find_if(
        d.cards(), [canonical](arcana::card const& c) { return c.canonical_id() == canonical; }
    );

    REQUIRE(at != d.cards().end());
    return *at;
}

}  // namespace

TEST_CASE("marseille composes its minors in its own words", "[loader][v2][names][reference-decks]")
{
    std::string const dir = REFERENCE_DECKS_DIR;
    if (dir.empty())
        SKIP("configured with ARCANA_FETCH_REFERENCE_DECKS=OFF");

    auto const loaded = arcana::load_deck(std::filesystem::path{dir} / "marseille");
    REQUIRE(loaded.has_value());

    // DECK.md#46-minor_arcana, DECK.md#44-suits and DECK.md#45-ranks
    CHECK(display_name_of(*loaded, "minor_arcana.cups.page") == "Valet de Coupe");
    CHECK(display_name_of(*loaded, "minor_arcana.swords.king") == "Roy d'Epee");
    CHECK(display_name_of(*loaded, "minor_arcana.pentacles.queen") == "Reyne de Deniers");

    // provisional: [ranks] names no `two`, and rc.1 is silent on a name the
    // template's layer does not give, so the fallback fills it
    CHECK(display_name_of(*loaded, "minor_arcana.wands.two") == "Two de Baton");

    CHECK(display_name_of(*loaded, "major_arcana.01") == "Le Bateleur");
}

TEST_CASE("marseille's unnamed and unnumbered faces", "[loader][v2][names][reference-decks]")
{
    std::string const dir = REFERENCE_DECKS_DIR;
    if (dir.empty())
        SKIP("configured with ARCANA_FETCH_REFERENCE_DECKS=OFF");

    auto const loaded = arcana::load_deck(std::filesystem::path{dir} / "marseille");
    REQUIRE(loaded.has_value());

    // DECK.md#73-display-name-resolution: the thirteenth prints no name
    auto const& thirteen = card_of(*loaded, "major_arcana.13");
    CHECK(thirteen.declared.unnamed);
    CHECK(thirteen.display_name.empty());

    // DECK.md#431-card-numbers: the Fool prints no number
    auto const& fool = card_of(*loaded, "major_arcana.00");
    CHECK(fool.declared.unnumbered);
    CHECK(fool.display_name == "Le Mat");

    CHECK(display_name_of(*loaded, "major_arcana.01") == "Le Bateleur");
}
