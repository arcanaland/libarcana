// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

// The prepared readings of the document and the tree that the checks share.

#include <facts.hpp>

#include <catch2/catch_test_macros.hpp>

#include <toml++/toml.hpp>

#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

using arcana::validation::card_entry;
using arcana::validation::deck_facts;
using arcana::validation::deck_file;

namespace
{

constexpr std::uint8_t v2 = 2;

// The facts of a manifest with no tree behind it.
deck_facts facts_of(toml::table const& doc)
{
    return deck_facts{std::span<deck_file const>{}, doc, v2};
}

toml::table parse(std::string_view text)
{
    auto result = toml::parse(text);
    REQUIRE(result.succeeded());

    return std::move(result).table();
}

card_entry const* entry_for(deck_facts const& facts, std::string_view reference)
{
    for (auto const& entry : facts.card_entries)
        if (entry.reference == reference)
            return &entry;

    return nullptr;
}

}  // namespace

TEST_CASE("a deck with no cards table indexes no entries", "[validation][facts]")
{
    auto const doc = parse("[deck]\nname = 'Nothing'\n");

    CHECK(facts_of(doc).card_entries.empty());
}

TEST_CASE("a plain card entry records the card it references", "[validation][facts]")
{
    auto const doc = parse(R"([cards."major_arcana.23"]
number = "XXIII"
position = 22
)");

    auto const facts = facts_of(doc);
    REQUIRE(facts.card_entries.size() == 1);

    auto const& entry = facts.card_entries.front();
    CHECK(entry.reference == "major_arcana.23");
    CHECK(entry.card == "major_arcana.23");
    CHECK_FALSE(entry.variant.has_value());
    CHECK_FALSE(entry.image.has_value());

    CHECK(entry.declares("number"));
    CHECK(entry.declares("position"));
    CHECK_FALSE(entry.declares("name"));
    CHECK_FALSE(entry.declares("default_variant"));
}

TEST_CASE("a variant reference records its suffix and its card", "[validation][facts]")
{
    auto const doc = parse(R"([cards."major_arcana.06"]
default_variant = "two_women"

[cards."major_arcana.06:two_women"]
name = "The Lovers"
image = "scalable/major_arcana/06.two_women.svg"
)");

    auto const facts = facts_of(doc);
    REQUIRE(facts.card_entries.size() == 2);

    auto const* variant = entry_for(facts, "major_arcana.06:two_women");
    REQUIRE(variant != nullptr);
    CHECK(variant->card == "major_arcana.06");
    REQUIRE(variant->variant.has_value());
    CHECK(*variant->variant == "two_women");

    // The declared image path the loaded model normalizes away.
    REQUIRE(variant->image.has_value());
    CHECK(*variant->image == "scalable/major_arcana/06.two_women.svg");

    auto const* card = entry_for(facts, "major_arcana.06");
    REQUIRE(card != nullptr);
    CHECK_FALSE(card->variant.has_value());
    CHECK(card->declares("default_variant"));
}

TEST_CASE("a malformed entry is indexed rather than judged", "[validation][facts]")
{
    // A key outside the card reference grammar, and an entry that is not a
    // table at all. Both are other codes' business to report.
    auto const doc = parse(R"(
[cards."Major Arcana 6"]
number = ""

[cards]
"major_arcana.07" = 7
)");

    auto const facts = facts_of(doc);
    REQUIRE(facts.card_entries.size() == 2);

    auto const* misspelt = entry_for(facts, "Major Arcana 6");
    REQUIRE(misspelt != nullptr);
    CHECK(misspelt->card == "Major Arcana 6");
    CHECK(misspelt->declares("number"));

    auto const* not_a_table = entry_for(facts, "major_arcana.07");
    REQUIRE(not_a_table != nullptr);
    CHECK(not_a_table->declared.empty());
    CHECK_FALSE(not_a_table->image.has_value());
}
