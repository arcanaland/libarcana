// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include "temp_dir.hpp"

#include <image_headers.hpp>

#include <arcana/card.hpp>
#include <arcana/deck.hpp>
#include <arcana/loader.hpp>

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

namespace
{

// The 22 canonical majors plus 4 suits of 14
constexpr std::size_t canonical_card_count = 78;

// A 2.0 deck carrying `manifest` as the body of deck.toml after [deck]
arcana_test::temp_dir make_deck(std::string_view manifest = "")
{
    arcana_test::temp_dir deck;
    deck.write(
        "deck.toml", std::format(
                         R"([deck]
schema_version = "2.0"
name = "Reader"
version = "1.0"
{}
)",
                         manifest
                     )
    );

    return deck;
}

arcana::deck load(arcana_test::temp_dir const& dir)
{
    auto result = arcana::load_deck(dir.path());
    REQUIRE(result.has_value());
    return *std::move(result);
}

std::vector<std::string> canonical_ids(arcana::deck const& d)
{
    std::vector<std::string> ids;
    ids.reserve(d.cards().size());
    for (auto const& c : d.cards()) ids.push_back(c.canonical_id());

    return ids;
}

bool has_card(arcana::deck const& d, std::string_view canonical)
{
    return std::ranges::any_of(
        d.cards(), [canonical](arcana::card const& c) { return c.canonical_id() == canonical; }
    );
}

arcana::card const& card_at(arcana::deck const& d, std::string_view canonical)
{
    auto const at = std::ranges::find_if(
        d.cards(), [canonical](arcana::card const& c) { return c.canonical_id() == canonical; }
    );

    REQUIRE(at != d.cards().end());
    return *at;
}

arcana::card_variant const& variant_at(arcana::card const& c, std::string_view key)
{
    auto const at = std::ranges::find(c.variants, key, &arcana::card_variant::key);

    REQUIRE(at != c.variants.end());
    return *at;
}

}  // namespace

TEST_CASE("a 2.0 deck always has the seventy-eight canonical slots", "[loader][v2]")
{
    auto const deck = load(make_deck());

    CHECK(deck.cards().size() == canonical_card_count);
    CHECK(deck.suits().size() == 4);

    // DECK.md#appendix-c-canonical-card-names-informative
    SECTION("majors fall back to the canonical names")
    {
        CHECK(card_at(deck, "major_arcana.00").display_name == "The Fool");
        CHECK(card_at(deck, "major_arcana.21").display_name == "The World");
    }

    SECTION("minors compose from the default template")
    {
        auto const& ace = card_at(deck, "minor_arcana.wands.ace");
        CHECK(ace.display_name == "Ace of Wands");
        CHECK(ace.display_rank == "Ace");
        CHECK(ace.display_suit == "Wands");
    }

    SECTION("a canonical suit carries the canonical rank sequence")
    {
        auto const& wands = deck.suits().front();
        CHECK(wands.key == "wands");
        CHECK(wands.standard);
        REQUIRE(wands.ranks.size() == 14);
        CHECK(wands.ranks.front() == "ace");
        CHECK(wands.ranks.back() == "king");
    }
}

TEST_CASE("a 1.0 table is not read by the 2.0 front end", "[loader][v2]")
{
    // [deck].author was removed for artist/creator, and v2 must not read it back
    auto const deck = load(make_deck(R"(author = "Nobody")"));

    CHECK_FALSE(deck.metadata().artist.has_value());
    CHECK_FALSE(deck.metadata().creator.has_value());
}

TEST_CASE("a 2.0 deck reports its identifier", "[loader][v2]")
{
    auto const deck = load(make_deck(R"(identifier = "net.example.jdoe/deck/reader")"));

    REQUIRE(deck.metadata().identifier.has_value());
    CHECK(*deck.metadata().identifier == "net.example.jdoe/deck/reader");
}

TEST_CASE("files create cards", "[loader][v2][discovery]")
{
    SECTION("a custom-keyed major")
    {
        auto dir = make_deck();
        dir.write("scalable/major_arcana/happy_squirrel.svg", "<svg/>");

        auto const deck = load(dir);

        CHECK(deck.cards().size() == canonical_card_count + 1);
        auto const& squirrel = card_at(deck, "major_arcana.happy_squirrel");
        CHECK(squirrel.display_name == "Happy Squirrel");
        CHECK(squirrel.images.size() == 1);
        CHECK_FALSE(squirrel.position.has_value());
    }

    SECTION("an extended major carries the number the deck prints")
    {
        auto dir = make_deck(R"(
[cards."major_arcana.23"]
number = "XXIII"
)");
        dir.write("h1200/major_arcana/23.png", arcana_test::png_header);

        auto const deck = load(dir);

        auto const& extended = card_at(deck, "major_arcana.23");
        REQUIRE(extended.number.has_value());
        CHECK(*extended.number == "XXIII");
        REQUIRE(extended.images.size() == 1);
        CHECK(extended.images.front().source_dir == "h1200");
        CHECK(extended.images.front().height == 1200);
    }

    SECTION("a custom suit")
    {
        auto dir = make_deck(R"(
[suits.stars]
name = "Stars"
ranks = ["ace", "two", "king"]
)");
        dir.write("scalable/minor_arcana/stars/ace.svg", "<svg/>");
        dir.write("scalable/minor_arcana/stars/king.svg", "<svg/>");

        auto const deck = load(dir);

        REQUIRE(deck.suits().size() == 5);
        auto const& stars = deck.suits().back();
        CHECK(stars.key == "stars");
        CHECK_FALSE(stars.standard);
        CHECK(stars.name == "Stars");
        CHECK(stars.ranks == std::vector<std::string>{"ace", "two", "king"});

        CHECK(has_card(deck, "minor_arcana.stars.ace"));
        CHECK(has_card(deck, "minor_arcana.stars.king"));

        // [suits] describes; the file is what creates a card
        CHECK_FALSE(has_card(deck, "minor_arcana.stars.two"));
    }

    SECTION("a directory with no files creates no suit")
    {
        auto const deck = load(make_deck(R"(
[suits.stars]
name = "Stars"
)"));

        CHECK(deck.suits().size() == 4);
    }
}

TEST_CASE("ranks on a canonical suit replace its canonical sequence", "[loader][v2]")
{
    auto const deck = load(make_deck(R"(
[suits.cups]
ranks = ["ace", "two", "princess", "king"]
)"));

    auto const at = std::ranges::find(deck.suits(), std::string{"cups"}, &arcana::suit_info::key);
    REQUIRE(at != deck.suits().end());
    CHECK(at->ranks == std::vector<std::string>{"ace", "two", "princess", "king"});
}

TEST_CASE("a [cards] entry does not create a card", "[loader][v2]")
{
    SECTION("but a canonical slot is accepted with no file at all")
    {
        auto const deck = load(make_deck(R"(
[cards."major_arcana.06"]
name = "The Lovers, renamed"
)"));

        CHECK(deck.cards().size() == canonical_card_count);
        CHECK(card_at(deck, "major_arcana.06").display_name == "The Lovers, renamed");
    }

    SECTION("and anything else needs a file")
    {
        auto const deck = load(make_deck(R"(
[cards."major_arcana.23"]
number = "XXIII"

[cards."major_arcana.happy_squirrel"]
name = "The Happy Squirrel"
)"));

        CHECK(deck.cards().size() == canonical_card_count);
        CHECK_FALSE(has_card(deck, "major_arcana.23"));
        CHECK_FALSE(has_card(deck, "major_arcana.happy_squirrel"));
    }
}

TEST_CASE("images resolve by the extension chain, not by filesystem order", "[loader][v2]")
{
    SECTION("png wins over jpeg for the same stem")
    {
        auto dir = make_deck();
        dir.write("h1200/major_arcana/00.jpeg", "jpeg");
        dir.write("h1200/major_arcana/00.png", "png");
        dir.write("h1200/major_arcana/00.avif", "avif");

        auto const deck = load(dir);

        auto const& fool = card_at(deck, "major_arcana.00");
        REQUIRE(fool.images.size() == 1);
        CHECK(fool.images.front().path.filename() == "00.png");
    }

    SECTION("an extension outside the chain is ignored entirely")
    {
        auto dir = make_deck();
        dir.write("h1200/major_arcana/00.tiff", "tiff");

        auto const deck = load(dir);

        CHECK(card_at(deck, "major_arcana.00").images.empty());
    }

    SECTION("an SVG is a card asset only under scalable/")
    {
        auto dir = make_deck();
        dir.write("h1200/major_arcana/00.svg", "<svg/>");
        dir.write("scalable/major_arcana/01.svg", "<svg/>");

        auto const deck = load(dir);

        CHECK(card_at(deck, "major_arcana.00").images.empty());
        CHECK(card_at(deck, "major_arcana.01").images.size() == 1);
    }

    SECTION("an ANSI root matches on stem alone and takes any extension or none")
    {
        auto dir = make_deck();
        dir.write("ansi20/major_arcana/00", "bare");
        dir.write("ansi20/major_arcana/01.ans", "ansi");
        dir.write("ansi20/major_arcana/02.txt", "text");

        auto const deck = load(dir);

        for (auto const* id : {"major_arcana.00", "major_arcana.01", "major_arcana.02"})
        {
            CAPTURE(id);
            auto const& c = card_at(deck, id);
            REQUIRE(c.images.size() == 1);
            CHECK(c.images.front().kind == arcana::image_kind::ansi);
            CHECK(c.images.front().lines == 20);
        }
    }

    SECTION("a variant file names its card but is not mistaken for one")
    {
        auto dir = make_deck();
        dir.write("scalable/major_arcana/06.svg", "<svg/>");
        dir.write("scalable/major_arcana/06.two_women.svg", "<svg/>");

        auto const deck = load(dir);

        CHECK(deck.cards().size() == canonical_card_count);
        CHECK_FALSE(has_card(deck, "major_arcana.06.two_women"));

        auto const& lovers = card_at(deck, "major_arcana.06");
        REQUIRE(lovers.images.size() == 2);
        CHECK_FALSE(lovers.images.front().variant_key.has_value());
        CHECK(lovers.images.back().variant_key == "two_women");
    }

    SECTION("a loose file and an unknown subdirectory are ignored")
    {
        auto dir = make_deck();
        dir.write("h1200/00.png", "png");
        dir.write("h1200/trumps/00.png", "png");
        dir.write("notes/00.png", "png");

        auto const deck = load(dir);

        CHECK(deck.cards().size() == canonical_card_count);
        CHECK(card_at(deck, "major_arcana.00").images.empty());
    }
}

TEST_CASE("an explicit image path wins over discovery", "[loader][v2]")
{
    auto dir = make_deck(R"(
[cards."major_arcana.00"]
image = "extra/fool.tiff"
)");
    dir.write("h1200/major_arcana/00.png", "png");
    dir.write("extra/fool.tiff", "tiff");

    auto const deck = load(dir);

    auto const& fool = card_at(deck, "major_arcana.00");
    REQUIRE(fool.images.size() == 1);
    CHECK(fool.images.front().path.filename() == "fool.tiff");
}

TEST_CASE("artwork variants belong to one card", "[loader][v2][variants]")
{
    SECTION("a declared default_variant is the artwork a bare reference resolves to")
    {
        auto dir = make_deck(R"(
[cards."major_arcana.06"]
default_variant = "two_women"
)");
        dir.write("scalable/major_arcana/06.svg", "<svg/>");
        dir.write("scalable/major_arcana/06.two_women.svg", "<svg/>");
        dir.write("scalable/major_arcana/06.two_men.svg", "<svg/>");

        auto const deck = load(dir);

        // One card, three artworks
        CHECK(deck.cards().size() == canonical_card_count);

        auto const& lovers = card_at(deck, "major_arcana.06");
        REQUIRE(lovers.images.size() == 3);
        CHECK(lovers.default_variant == "two_women");
        CHECK(lovers.variant_keys() == std::vector<std::string>{"two_men", "two_women"});

        auto const scalable = lovers.scalable_image();
        REQUIRE(scalable.has_value());
        CHECK(scalable->path.filename() == "06.two_women.svg");

        auto const requested = lovers.images_for_variant("two_men");
        REQUIRE(requested.size() == 1);
        CHECK(requested.front().path.filename() == "06.two_men.svg");

        // DECK.md#675-variants: a variant the card lacks resolves to its default, not an error
        auto const missing = lovers.images_for_variant("no_such_key");
        REQUIRE(missing.size() == 1);
        CHECK(missing.front().path.filename() == "06.two_women.svg");
    }

    SECTION("a card with only variant files and no default_variant still loads")
    {
        auto dir = make_deck();
        dir.write("scalable/major_arcana/06.two_women.svg", "<svg/>");
        dir.write("scalable/major_arcana/06.two_men.svg", "<svg/>");

        auto const deck = load(dir);

        auto const& lovers = card_at(deck, "major_arcana.06");
        REQUIRE(lovers.images.size() == 2);

        // The lexicographically first variant, which a validator may object to
        CHECK(lovers.default_variant == "two_men");

        auto const scalable = lovers.scalable_image();
        REQUIRE(scalable.has_value());
        CHECK(scalable->path.filename() == "06.two_men.svg");
    }

    SECTION("the unsuffixed file is the default where the deck declares none")
    {
        auto dir = make_deck();
        dir.write("h1200/major_arcana/06.png", "png");
        dir.write("h1200/major_arcana/06.two_women.png", "png");

        auto const deck = load(dir);

        auto const& lovers = card_at(deck, "major_arcana.06");
        CHECK_FALSE(lovers.default_variant.has_value());

        // A variant does not leak into a bare request for the card's artwork
        auto const raster = lovers.best_raster_for_height(1200);
        REQUIRE(raster.has_value());
        CHECK(raster->path.filename() == "06.png");

        auto const variant = lovers.best_raster_for_height(1200, "two_women");
        REQUIRE(variant.has_value());
        CHECK(variant->path.filename() == "06.two_women.png");
    }

    SECTION("a variant entry's image creates the variant, and a card's replaces only the default")
    {
        auto dir = make_deck(R"(
[cards."major_arcana.06"]
image = "extra/lovers.tiff"

[cards."major_arcana.06:two_women"]
image = "extra/two_women.tiff"
)");
        dir.write("scalable/major_arcana/06.svg", "<svg/>");
        dir.write("scalable/major_arcana/06.two_men.svg", "<svg/>");
        dir.write("extra/lovers.tiff", "tiff");
        dir.write("extra/two_women.tiff", "tiff");

        auto const deck = load(dir);

        auto const& lovers = card_at(deck, "major_arcana.06");
        REQUIRE(lovers.images.size() == 3);
        CHECK(lovers.variant_keys() == std::vector<std::string>{"two_men", "two_women"});

        // The card's `image` wins over the unsuffixed file and leaves the
        // discovered variant alone
        REQUIRE(lovers.images.front().path.filename() == "lovers.tiff");
        CHECK_FALSE(lovers.images.front().variant_key.has_value());

        auto const declared = lovers.images_for_variant("two_women");
        REQUIRE(declared.size() == 1);
        CHECK(declared.front().path.filename() == "two_women.tiff");
        CHECK(lovers.images_for_variant("two_men").front().path.filename() == "06.two_men.svg");
    }

    // DECK.md#43-cards
    SECTION("card-level keys are ignored on a variant reference")
    {
        auto dir = make_deck(R"(
[cards."major_arcana.06:two_women"]
number = "XCIX"
position = 3
default_variant = "two_men"
)");
        dir.write("scalable/major_arcana/06.svg", "<svg/>");
        dir.write("scalable/major_arcana/06.two_women.svg", "<svg/>");

        auto const deck = load(dir);
        auto const ids = canonical_ids(deck);

        auto const& lovers = card_at(deck, "major_arcana.06");
        CHECK_FALSE(lovers.number.has_value());
        CHECK_FALSE(lovers.position.has_value());
        CHECK_FALSE(lovers.default_variant.has_value());

        // The card kept its place, so `position` did not reach it
        CHECK(ids[6] == "major_arcana.06");
    }
}

TEST_CASE("a variant is an entity with its own strings", "[loader][v2][variants]")
{
    SECTION("a manifest entry names the variant")
    {
        auto dir = make_deck(R"(
[cards."major_arcana.06:two_women"]
name = "The Lovers, Two Women"
alt_text = "Two women beneath an angel."
)");
        dir.write("scalable/major_arcana/06.svg", "<svg/>");
        dir.write("scalable/major_arcana/06.two_women.svg", "<svg/>");

        auto const deck = load(dir);
        auto const& lovers = card_at(deck, "major_arcana.06");

        REQUIRE(lovers.variants.size() == 1);

        auto const& two_women = variant_at(lovers, "two_women");
        CHECK(two_women.display_name == "The Lovers, Two Women");
        CHECK(two_women.alt_text == "Two women beneath an angel.");
    }

    SECTION("a name file wins over the manifest")
    {
        auto dir = make_deck(R"(
[cards."major_arcana.06:two_women"]
name = "The Lovers, Two Women"
alt_text = "Two women beneath an angel."
)");
        dir.write("scalable/major_arcana/06.svg", "<svg/>");
        dir.write("scalable/major_arcana/06.two_women.svg", "<svg/>");
        dir.write("names/en.toml", R"(
[name.variant]
"major_arcana.06:two_women" = "Les Amoureuses"

[alt_text.variant]
"major_arcana.06:two_women" = "Deux femmes sous un ange."
)");

        auto const deck = load(dir);
        auto const& lovers = card_at(deck, "major_arcana.06");
        auto const& two_women = variant_at(lovers, "two_women");

        CHECK(two_women.display_name == "Les Amoureuses");
        CHECK(two_women.alt_text == "Deux femmes sous un ange.");
    }

    SECTION("a variant the deck names nowhere takes the card's own strings")
    {
        auto dir = make_deck(R"(
[cards."major_arcana.06"]
alt_text = "Two figures beneath an angel."
)");
        dir.write("scalable/major_arcana/06.svg", "<svg/>");
        dir.write("scalable/major_arcana/06.two_women.svg", "<svg/>");
        dir.write("scalable/major_arcana/06.two_men.svg", "<svg/>");

        auto const deck = load(dir);
        auto const& lovers = card_at(deck, "major_arcana.06");

        // Sorted by key, one per variant_keys() entry
        REQUIRE(lovers.variants.size() == 2);
        CHECK(lovers.variants.front().key == "two_men");
        CHECK(lovers.variants.back().key == "two_women");

        // The card's resolved name, not a title-cased key
        for (auto const& variant : lovers.variants)
        {
            CAPTURE(variant.key);
            CHECK(variant.display_name == "The Lovers");
            CHECK(variant.alt_text == "Two figures beneath an angel.");
        }
    }

    SECTION("a variant entry for a key with no artwork makes no variant")
    {
        auto dir = make_deck(R"(
[cards."major_arcana.06:two_women"]
name = "The Lovers, Two Women"
)");
        dir.write("scalable/major_arcana/06.svg", "<svg/>");

        auto const deck = load(dir);
        auto const& lovers = card_at(deck, "major_arcana.06");

        CHECK(lovers.variant_keys().empty());
        CHECK(lovers.variants.empty());
    }

    SECTION("a variant entry carrying an image creates the variant it names")
    {
        auto dir = make_deck(R"(
[cards."major_arcana.06:two_women"]
name = "The Lovers, Two Women"
image = "extra/two_women.tiff"
)");
        dir.write("scalable/major_arcana/06.svg", "<svg/>");
        dir.write("extra/two_women.tiff", "tiff");

        auto const deck = load(dir);
        auto const& lovers = card_at(deck, "major_arcana.06");

        REQUIRE(lovers.variants.size() == 1);
        CHECK(variant_at(lovers, "two_women").display_name == "The Lovers, Two Women");
    }

    SECTION("origin inherits one link at a time")
    {
        auto dir = make_deck(R"(
[deck.origin]
"iptc-dst" = "print"
"x_studio" = "house"

[cards."major_arcana.06"]
origin = { "x_studio" = "guest" }

[cards."major_arcana.06:two_women"]
origin = { "iptc-dst" = "trainedAlgorithmicMedia" }
)");
        dir.write("scalable/major_arcana/06.svg", "<svg/>");
        dir.write("scalable/major_arcana/06.two_women.svg", "<svg/>");
        dir.write("scalable/major_arcana/06.two_men.svg", "<svg/>");

        auto const deck = load(dir);
        auto const& lovers = card_at(deck, "major_arcana.06");

        // Sorted by system, as everywhere else origin is resolved
        CHECK(
            variant_at(lovers, "two_women").origin ==
            std::vector<arcana::origin_term>{
                {.system = "iptc-dst", .term = "trainedAlgorithmicMedia"},
                {.system = "x_studio", .term = "guest"}
            }
        );

        // A variant that declares nothing is its card's, deck's link included
        CHECK(
            variant_at(lovers, "two_men").origin ==
            std::vector<arcana::origin_term>{
                {.system = "iptc-dst", .term = "print"}, {.system = "x_studio", .term = "guest"}
            }
        );
    }

    SECTION("a card with no variants carries none")
    {
        auto dir = make_deck();
        dir.write("scalable/major_arcana/06.svg", "<svg/>");

        auto const deck = load(dir);
        CHECK(card_at(deck, "major_arcana.06").variants.empty());
    }
}

// DECK.md#5-ordering
TEST_CASE("cards come out in the specification's order", "[loader][v2][ordering]")
{
    SECTION("majors by position, then minors by suit and rank")
    {
        auto const deck = load(make_deck());
        auto const ids = canonical_ids(deck);

        REQUIRE(ids.size() == canonical_card_count);
        CHECK(ids.front() == "major_arcana.00");
        CHECK(ids[21] == "major_arcana.21");
        CHECK(ids[22] == "minor_arcana.wands.ace");
        CHECK(ids[35] == "minor_arcana.wands.king");
        CHECK(ids[36] == "minor_arcana.cups.ace");
        CHECK(ids.back() == "minor_arcana.pentacles.king");
    }

    SECTION("a declared position places a custom major, and one without follows")
    {
        auto dir = make_deck(R"(
[cards."major_arcana.the_morning"]
position = 2
)");
        dir.write("scalable/major_arcana/the_morning.svg", "<svg/>");
        dir.write("scalable/major_arcana/the_evening.svg", "<svg/>");

        auto const deck = load(dir);
        auto const ids = canonical_ids(deck);

        // A declared position precedes the implicit one of the same value
        CHECK(ids[2] == "major_arcana.the_morning");
        CHECK(ids[3] == "major_arcana.02");

        // A custom key with no position follows every card that has one
        CHECK(ids[23] == "major_arcana.the_evening");
    }

    SECTION("a suit's ranks sequence orders its cards")
    {
        auto dir = make_deck(R"(
[suits.stars]
ranks = ["king", "ace"]
)");
        dir.write("scalable/minor_arcana/stars/ace.svg", "<svg/>");
        dir.write("scalable/minor_arcana/stars/king.svg", "<svg/>");
        dir.write("scalable/minor_arcana/stars/knight.svg", "<svg/>");

        auto const deck = load(dir);
        auto const stars = deck.cards_in_suit("stars");

        REQUIRE(stars.size() == 3);
        CHECK(stars[0].canonical_id() == "minor_arcana.stars.king");
        CHECK(stars[1].canonical_id() == "minor_arcana.stars.ace");

        // A rank the sequence does not name follows every rank it does
        CHECK(stars[2].canonical_id() == "minor_arcana.stars.knight");
    }

    SECTION("a custom suit follows the four canonical ones")
    {
        auto dir = make_deck();
        dir.write("scalable/minor_arcana/stars/ace.svg", "<svg/>");

        auto const deck = load(dir);

        CHECK(canonical_ids(deck).back() == "minor_arcana.stars.ace");
    }
}

TEST_CASE("excluded cards are not loaded", "[loader][v2]")
{
    auto const deck = load(make_deck(R"(
[excluded_cards]
cards = ["minor_arcana.pentacles.page", "minor_arcana.pentacles.knight"]
reason = "This deck excludes these specific court cards."
)"));

    CHECK(deck.cards().size() == canonical_card_count - 2);
    CHECK_FALSE(has_card(deck, "minor_arcana.pentacles.page"));
    REQUIRE(deck.exclusion_reason("minor_arcana.pentacles.page").has_value());
}

TEST_CASE("card back designs are discovered from the directory structure", "[loader][v2]")
{
    SECTION("a file alone defines a design")
    {
        auto dir = make_deck();
        dir.write("card_backs/classic.png", "png");

        auto const deck = load(dir);

        REQUIRE(deck.card_backs().size() == 1);
        auto const& classic = deck.card_backs().front();
        CHECK(classic.id == "classic");
        CHECK(classic.name == "Classic");
        CHECK_FALSE(classic.declared);
        CHECK(classic.image.filename() == "classic.png");
    }

    SECTION("an image root supplies the design at that root's kind")
    {
        auto dir = make_deck(R"(
[card_backs]
default = "classic"

[card_backs.designs.classic]
name = "Classic RWS Back"
)");
        dir.write("ansi6/card_backs/classic.txt", "text");

        auto const deck = load(dir);

        REQUIRE(deck.card_backs().size() == 1);
        CHECK(deck.card_backs().front().declared);
        CHECK(deck.card_backs().front().name == "Classic RWS Back");

        auto const chosen = deck.default_card_back_design();
        REQUIRE(chosen.has_value());
        CHECK(chosen->id == "classic");
    }

    SECTION("a stem holding a dot names no design")
    {
        auto dir = make_deck();
        dir.write("card_backs/classic.dark.png", "png");

        auto const deck = load(dir);

        CHECK(deck.card_backs().empty());
    }
}

TEST_CASE("name files are read in the 2.0 facet layout", "[loader][v2][names]")
{
    auto dir = make_deck(R"(
[suits.stars]
ranks = ["ace"]
)");
    dir.write("scalable/minor_arcana/stars/ace.svg", "<svg/>");
    dir.write("card_backs/classic.png", "png");
    dir.write("names/en.toml", R"(
[name.card.major_arcana]
08 = "Justice"

[name.card.minor_arcana]
name_template = "{rank} des {suit}"

[name.card.minor_arcana.cups]
ace = "The Ace of Cups"

[name.suit]
stars = "Étoiles"

[name.rank]
knight = "Warrior"

[name.card_back]
classic = "Classic Back"

[alt_text.card.major_arcana]
00 = "A young person steps off a cliff."
)");

    auto const deck = load(dir);

    SECTION("a name file wins over every fallback")
    {
        CHECK(card_at(deck, "major_arcana.08").display_name == "Justice");
        CHECK(card_at(deck, "minor_arcana.cups.ace").display_name == "The Ace of Cups");
        CHECK(deck.card_backs().front().name == "Classic Back");
    }

    SECTION("suit and rank names resolve from the name file")
    {
        CHECK(deck.display_suit_name("stars") == "Étoiles");
        CHECK(deck.display_rank_name(arcana::rank::knight) == "Warrior");

        // A key the deck does not name falls back to its title-cased form
        CHECK(deck.display_rank_name(arcana::rank::queen) == "Queen");
    }

    SECTION("the template composes the minors the file does not name")
    {
        // provisional: the file names no `wands`, and rc.1 is silent on a name
        // the template's layer does not give, so the fallback fills it
        CHECK(card_at(deck, "minor_arcana.wands.ace").display_name == "Ace des Wands");
        CHECK(card_at(deck, "minor_arcana.swords.knight").display_name == "Warrior des Swords");
        CHECK(card_at(deck, "minor_arcana.stars.ace").display_name == "Ace des Étoiles");
    }

    SECTION("alt text comes from its own facet")
    {
        REQUIRE(card_at(deck, "major_arcana.00").alt_text.has_value());
        CHECK_FALSE(card_at(deck, "major_arcana.01").alt_text.has_value());
    }
}

TEST_CASE("a name template leaves undefined placeholders alone", "[loader][v2][names]")
{
    auto dir = make_deck();
    dir.write("names/en.toml", R"(
[name.card.minor_arcana]
name_template = "{rank} of {suit} {unknown}"
)");

    auto const deck = load(dir);

    CHECK(card_at(deck, "minor_arcana.cups.two").display_name == "Two of Cups {unknown}");
}

TEST_CASE("a manifest name is the fallback a name file overrides", "[loader][v2][names]")
{
    auto const deck = load(make_deck(R"(
[cards."major_arcana.00"]
name = "Le Mat"

[suits.wands]
name = "Batons"
)"));

    CHECK(card_at(deck, "major_arcana.00").display_name == "Le Mat");
    CHECK(deck.display_suit_name(arcana::suit::wands) == "Batons");
}

TEST_CASE("the manifest's naming declarations are kept", "[loader][v2][names]")
{
    auto dir = make_deck(R"(
[minor_arcana]
name_template = "{rank} de {suit}"

[suits]
wands = { name = "Baton" }
swords = { name = "Epee", name_template = "{rank} d'{suit}" }

[suits.stars]
ranks = ["ace", "valet"]

[ranks]
page = { name = "Valet" }
valet = { name = "Varlet" }
unused = { name = "Nobody" }
)");
    dir.write("scalable/minor_arcana/stars/ace.svg", "<svg/>");
    dir.write("scalable/minor_arcana/stars/valet.svg", "<svg/>");
    // A custom suit with no `ranks` sequence, so only its file creates the rank
    dir.write("scalable/minor_arcana/moons/princess.svg", "<svg/>");
    dir.write("names/en.toml", R"(
[name.suit]
wands = "Wands"
)");

    auto const deck = load(dir);

    auto const suit_named = [&](std::string_view key) -> arcana::suit_info const&
    {
        auto const at = std::ranges::find(deck.suits(), key, &arcana::suit_info::key);
        REQUIRE(at != deck.suits().end());
        return *at;
    };

    auto const rank_named = [&](std::string_view key) -> arcana::rank_info const&
    {
        auto const at = std::ranges::find(deck.ranks(), key, &arcana::rank_info::key);
        REQUIRE(at != deck.ranks().end());
        return *at;
    };

    SECTION("[minor_arcana].name_template is read")
    {
        REQUIRE(deck.minor_arcana_name_template().has_value());
        CHECK(*deck.minor_arcana_name_template() == "{rank} de {suit}");
    }

    SECTION("a suit's declaration is read")
    {
        auto const& swords = suit_named("swords");
        CHECK(swords.declared.name == "Epee");
        CHECK(swords.declared.name_template == "{rank} d'{suit}");

        auto const& cups = suit_named("cups");
        CHECK_FALSE(cups.declared.name.has_value());
        CHECK_FALSE(cups.declared.name_template.has_value());
    }

    SECTION("the declared suit name survives a name file overriding it")
    {
        auto const& wands = suit_named("wands");
        CHECK(wands.name == "Wands");
        CHECK(wands.declared.name == "Baton");
    }

    SECTION("a rank's declaration is read")
    {
        CHECK(rank_named("page").declared.name == "Valet");
        CHECK(rank_named("valet").declared.name == "Varlet");
        CHECK_FALSE(rank_named("ace").declared.name.has_value());
    }

    SECTION("ranks lists the suits' sequences in order, then keys only a file uses")
    {
        std::vector<std::string> keys;
        for (auto const& info : deck.ranks()) keys.push_back(info.key);

        CHECK(
            keys == std::vector<std::string>{
                        "ace", "two", "three", "four", "five", "six", "seven", "eight", "nine",
                        "ten", "page", "knight", "queen", "king", "valet", "princess"
                    }
        );
    }

    SECTION("a [ranks] key no suit or file uses is not a rank of the deck")
    {
        CHECK(
            std::ranges::find(deck.ranks(), std::string_view{"unused"}, &arcana::rank_info::key) ==
            deck.ranks().end()
        );
    }
}

TEST_CASE(
    "suit and rank names resolve name file, then manifest, then fallback", "[loader][v2][names]"
)
{
    auto dir = make_deck(R"(
[suits]
cups = { name = "Coupe" }
wands = { name = "Baton" }

[suits.stars]
ranks = ["ace", "valet"]

[ranks]
page = { name = "Valet" }
king = { name = "Roy" }
)");
    dir.write("scalable/minor_arcana/stars/ace.svg", "<svg/>");
    dir.write("scalable/minor_arcana/stars/valet.svg", "<svg/>");
    dir.write("names/en.toml", R"(
[name.suit]
wands = "Wands"

[name.rank]
king = "King"
)");

    auto const deck = load(dir);

    auto const rank_named = [&](std::string_view key) -> arcana::rank_info const&
    {
        auto const at = std::ranges::find(deck.ranks(), key, &arcana::rank_info::key);
        REQUIRE(at != deck.ranks().end());
        return *at;
    };

    SECTION("[ranks] names a rank")
    {
        CHECK(deck.display_rank_name(arcana::rank::page) == "Valet");
        CHECK(rank_named("page").name == "Valet");
        CHECK(card_at(deck, "minor_arcana.cups.page").display_rank == "Valet");
    }

    SECTION("a name file's [name.rank] outranks [ranks]")
    {
        CHECK(deck.display_rank_name(arcana::rank::king) == "King");
        CHECK(rank_named("king").name == "King");
        CHECK(rank_named("king").declared.name == "Roy");
    }

    SECTION("a custom rank named nowhere is title-cased")
    {
        CHECK(deck.display_rank_name("valet") == "Valet");
        CHECK(rank_named("valet").name == "Valet");
    }

    SECTION("a suit takes its manifest name unless a name file gives one")
    {
        CHECK(deck.display_suit_name(arcana::suit::cups) == "Coupe");
        CHECK(deck.display_suit_name(arcana::suit::wands) == "Wands");
        CHECK(deck.display_suit_name("stars") == "Stars");
    }
}

TEST_CASE("minor names compose by the five-step chain", "[loader][v2][names]")
{
    // DECK.md#731-minor-arcana-name-composition
    auto const manifest = R"(
[minor_arcana]
name_template = "{rank} de {suit}"

[suits]
wands = { name = "Baton" }
cups = { name = "Coupe" }
swords = { name = "Epee", name_template = "{rank} d'{suit}" }

[ranks]
ace = { name = "As" }
page = { name = "Valet" }
king = { name = "Roy" }
)";

    SECTION("step 1 beats step 2, and step 2 beats steps 3 and 4")
    {
        auto dir = make_deck(manifest);
        dir.write("names/en.toml", R"(
[name.card.minor_arcana]
name_template = "{rank} of the {suit}"

[name.card.minor_arcana.swords]
name_template = "{suit}: {rank}"

[name.suit]
wands = "Rods"
swords = "Blades"

[name.rank]
ace = "One"
)");

        auto const deck = load(dir);

        CHECK(card_at(deck, "minor_arcana.swords.ace").display_name == "Blades: One");
        CHECK(card_at(deck, "minor_arcana.wands.ace").display_name == "One of the Rods");
    }

    SECTION("step 3 beats step 4 for its suit only")
    {
        auto const deck = load(make_deck(manifest));

        CHECK(card_at(deck, "minor_arcana.swords.king").display_name == "Roy d'Epee");
        CHECK(card_at(deck, "minor_arcana.cups.page").display_name == "Valet de Coupe");
        CHECK(card_at(deck, "minor_arcana.wands.ace").display_name == "As de Baton");
    }

    SECTION("step 5 with no template anywhere composes the resolved names")
    {
        auto dir = make_deck(R"(
[suits]
cups = { name = "Coupe" }

[ranks]
page = { name = "Valet" }
)");
        dir.write("names/en.toml", R"(
[name.rank]
king = "Rex"
)");

        auto const deck = load(dir);

        CHECK(card_at(deck, "minor_arcana.cups.page").display_name == "Valet of Coupe");
        CHECK(card_at(deck, "minor_arcana.cups.king").display_name == "Rex of Coupe");
        CHECK(card_at(deck, "minor_arcana.wands.two").display_name == "Two of Wands");
    }

    SECTION("a template composes with names from its own layer")
    {
        auto dir = make_deck(manifest);
        dir.write("names/en.toml", R"(
[name.suit]
cups = "Cups"
)");

        auto const deck = load(dir);
        auto const& page = card_at(deck, "minor_arcana.cups.page");

        // rc.1 breaks display_name == template(display_rank, display_suit) on
        // purpose: the manifest's template joins the manifest's words, while
        // the suit's own display name comes from the name file
        CHECK(page.display_name == "Valet de Coupe");
        CHECK(page.display_suit == "Cups");
    }

    SECTION("provisional: a manifest template meets a rank the manifest does not name")
    {
        // rc.1 is silent on a name the template's layer does not give: the
        // fallback, not the name file's word
        auto dir = make_deck(manifest);
        dir.write("names/en.toml", R"(
[name.rank]
two = "Deuce"
)");

        auto const deck = load(dir);

        CHECK(card_at(deck, "minor_arcana.wands.two").display_name == "Two de Baton");
    }

    SECTION("a [cards] name beats every template")
    {
        auto const deck = load(make_deck(
            std::format(
                R"({}
[cards."minor_arcana.cups.ace"]
name = "L'As de Coupe"
)",
                manifest
            )
        ));

        CHECK(card_at(deck, "minor_arcana.cups.ace").display_name == "L'As de Coupe");
    }
}

TEST_CASE("a 1.0 deck declares nothing", "[loader][v1][names]")
{
    arcana_test::temp_dir dir;
    dir.write("deck.toml", R"([deck]
schema_version = "1.0"
name = "Old"
version = "1.0"

[aliases.courts]
page = "Princess"
)");

    auto const deck = load(dir);

    CHECK_FALSE(deck.minor_arcana_name_template().has_value());

    for (auto const& info : deck.suits())
    {
        CHECK_FALSE(info.declared.name.has_value());
        CHECK_FALSE(info.declared.name_template.has_value());
    }

    REQUIRE(deck.ranks().size() == 14);
    CHECK(deck.ranks().front().key == "ace");
    for (auto const& info : deck.ranks()) CHECK_FALSE(info.declared.name.has_value());

    auto const page =
        std::ranges::find(deck.ranks(), std::string_view{"page"}, &arcana::rank_info::key);
    REQUIRE(page != deck.ranks().end());
    CHECK(page->name == "Princess");
}

TEST_CASE("artwork origin is resolved at the door", "[loader][v2][origin]")
{
    auto dir = make_deck(R"(
[deck.origin]
"iptc-dst" = "print"

[cards."major_arcana.13"]
origin = { "iptc-dst" = "compositeWithTrainedAlgorithmicMedia" }

[card_backs.designs.classic.origin]
"iptc-dst" = "digitalCreation"
)");
    dir.write("card_backs/classic.png", "png");

    auto const deck = load(dir);

    auto const term_for =
        [](std::vector<arcana::origin_term> const& terms, std::string const& system)
    {
        auto const at = std::ranges::find(terms, system, &arcana::origin_term::system);
        REQUIRE(at != terms.end());
        return at->term;
    };

    SECTION("the deck states its own")
    {
        CHECK(term_for(deck.metadata().origin, "iptc-dst") == "print");
    }

    SECTION("a card that declares nothing inherits the deck's")
    {
        CHECK(term_for(card_at(deck, "major_arcana.00").origin, "iptc-dst") == "print");
        CHECK(term_for(card_at(deck, "minor_arcana.cups.ace").origin, "iptc-dst") == "print");
    }

    SECTION("a card that declares its own keeps it")
    {
        CHECK(
            term_for(card_at(deck, "major_arcana.13").origin, "iptc-dst") ==
            "compositeWithTrainedAlgorithmicMedia"
        );
    }

    SECTION("a back design overrides independently of the cards")
    {
        REQUIRE(deck.card_backs().size() == 1);
        CHECK(term_for(deck.card_backs().front().origin, "iptc-dst") == "digitalCreation");
    }

    SECTION("a deck that declares no origin gives its cards none")
    {
        auto const bare = load(make_deck());

        CHECK(bare.metadata().origin.empty());
        CHECK(card_at(bare, "major_arcana.00").origin.empty());
    }
}
