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
#include <vector>

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

TEST_CASE("marseille's back declares itself reversible", "[loader][v2][declared][reference-decks]")
{
    std::string const dir = REFERENCE_DECKS_DIR;
    if (dir.empty())
        SKIP("configured with ARCANA_FETCH_REFERENCE_DECKS=OFF");

    auto const loaded = arcana::load_deck(std::filesystem::path{dir} / "marseille");
    REQUIRE(loaded.has_value());

    auto const back = loaded->default_card_back_design();
    REQUIRE(back.has_value());
    CHECK(back->id == "default");
    CHECK(back->declaration.reversible == true);
}

TEST_CASE("marseille's [deck] keys are kept as declared", "[loader][v2][metadata][reference-decks]")
{
    std::string const dir = REFERENCE_DECKS_DIR;
    if (dir.empty())
        SKIP("configured with ARCANA_FETCH_REFERENCE_DECKS=OFF");

    auto const loaded = arcana::load_deck(std::filesystem::path{dir} / "marseille");
    REQUIRE(loaded.has_value());

    auto const& metadata = loaded->metadata();
    CHECK(metadata.pips == arcana::pip_style::emblematic);
    CHECK(metadata.redistribution == arcana::sharing_terms::full);
    CHECK(metadata.derivation == arcana::sharing_terms::full);
    CHECK(metadata.metadata_language == "en");
    CHECK(metadata.artwork_language == std::vector<std::string>{"fr-1694acad"});
    CHECK(metadata.published_date == "2010");
    CHECK(metadata.rights_status == "https://creativecommons.org/licenses/by-nc-sa/3.0/");

    REQUIRE(metadata.links.size() == 3);
    CHECK(metadata.links[0].rel == arcana::link_rel::homepage);
    CHECK(metadata.links[1].rel == arcana::link_rel::buy);
    CHECK(metadata.links[2].rel == arcana::link_rel::source);
}

TEST_CASE(
    "rider-waite-smith's [deck] keys are kept as declared",
    "[loader][v2][metadata][reference-decks]"
)
{
    std::string const dir = REFERENCE_DECKS_DIR;
    if (dir.empty())
        SKIP("configured with ARCANA_FETCH_REFERENCE_DECKS=OFF");

    auto const loaded = arcana::load_deck(std::filesystem::path{dir} / "rider-waite-smith");
    REQUIRE(loaded.has_value());

    CHECK(loaded->metadata().published_date == "1909-12-01");
    CHECK(loaded->metadata().license_files == std::vector<std::string>{"LICENSE"});
}

TEST_CASE(
    "ascii-tarot's [deck] keys are kept as declared", "[loader][v2][metadata][reference-decks]"
)
{
    std::string const dir = REFERENCE_DECKS_DIR;
    if (dir.empty())
        SKIP("configured with ARCANA_FETCH_REFERENCE_DECKS=OFF");

    auto const loaded = arcana::load_deck(std::filesystem::path{dir} / "ascii-tarot");
    REQUIRE(loaded.has_value());

    CHECK(loaded->metadata().pips == arcana::pip_style::scenic);

    auto const& links = loaded->metadata().links;
    auto const artist =
        std::ranges::find(links, std::string_view{"artist"}, &arcana::deck_link::rel_name);
    REQUIRE(artist != links.end());
    CHECK(artist->rel == arcana::link_rel::artist);
}
