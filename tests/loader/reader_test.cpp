// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include "temp_dir.hpp"

#include <arcana/card.hpp>
#include <arcana/deck.hpp>

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
    ids.reserve(d.cards.size());
    for (auto const& c : d.cards) ids.push_back(c.canonical_id());

    return ids;
}

bool has_card(arcana::deck const& d, std::string_view canonical)
{
    return std::ranges::any_of(
        d.cards, [canonical](arcana::card const& c) { return c.canonical_id() == canonical; }
    );
}

arcana::card const& card_at(arcana::deck const& d, std::string_view canonical)
{
    auto const at = std::ranges::find_if(
        d.cards, [canonical](arcana::card const& c) { return c.canonical_id() == canonical; }
    );

    REQUIRE(at != d.cards.end());
    return *at;
}

}  // namespace

TEST_CASE("a 2.0 deck always has the seventy-eight canonical slots", "[loader][v2]")
{
    auto const deck = load(make_deck());

    CHECK(deck.cards.size() == canonical_card_count);
    CHECK(deck.suits.size() == 4);

    SECTION("majors fall back to the Appendix C names")
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
        auto const& wands = deck.suits.front();
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

    CHECK_FALSE(deck.metadata.artist.has_value());
    CHECK_FALSE(deck.metadata.creator.has_value());
}

TEST_CASE("a 2.0 deck reports its identifier", "[loader][v2]")
{
    auto const deck = load(make_deck(R"(identifier = "net.example.jdoe/deck/reader")"));

    REQUIRE(deck.metadata.identifier.has_value());
    CHECK(*deck.metadata.identifier == "net.example.jdoe/deck/reader");
}

TEST_CASE("files create cards", "[loader][v2][discovery]")
{
    SECTION("a custom-keyed major")
    {
        auto dir = make_deck();
        dir.write("scalable/major_arcana/happy_squirrel.svg", "<svg/>");

        auto const deck = load(dir);

        CHECK(deck.cards.size() == canonical_card_count + 1);
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
        dir.write("h1200/major_arcana/23.png", "\x89PNG");

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

        REQUIRE(deck.suits.size() == 5);
        auto const& stars = deck.suits.back();
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

        CHECK(deck.suits.size() == 4);
    }
}

TEST_CASE("ranks on a canonical suit replace its canonical sequence", "[loader][v2]")
{
    auto const deck = load(make_deck(R"(
[suits.cups]
ranks = ["ace", "two", "princess", "king"]
)"));

    auto const at = std::ranges::find(deck.suits, std::string{"cups"}, &arcana::suit_info::key);
    REQUIRE(at != deck.suits.end());
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

        CHECK(deck.cards.size() == canonical_card_count);
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

        CHECK(deck.cards.size() == canonical_card_count);
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

        CHECK(deck.cards.size() == canonical_card_count);
        CHECK_FALSE(has_card(deck, "major_arcana.06.two_women"));

        // Layer 5 gives card_image a variant key; until then only the
        // unsuffixed file supplies artwork
        CHECK(card_at(deck, "major_arcana.06").images.size() == 1);
    }

    SECTION("a loose file and an unknown subdirectory are ignored")
    {
        auto dir = make_deck();
        dir.write("h1200/00.png", "png");
        dir.write("h1200/trumps/00.png", "png");
        dir.write("notes/00.png", "png");

        auto const deck = load(dir);

        CHECK(deck.cards.size() == canonical_card_count);
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

TEST_CASE("cards come out in the order of §4.3.2", "[loader][v2][ordering]")
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

    CHECK(deck.cards.size() == canonical_card_count - 2);
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

        REQUIRE(deck.card_backs.size() == 1);
        auto const& classic = deck.card_backs.front();
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

        REQUIRE(deck.card_backs.size() == 1);
        CHECK(deck.card_backs.front().declared);
        CHECK(deck.card_backs.front().name == "Classic RWS Back");

        auto const chosen = deck.default_card_back_design();
        REQUIRE(chosen.has_value());
        CHECK(chosen->id == "classic");
    }

    SECTION("a stem holding a dot names no design")
    {
        auto dir = make_deck();
        dir.write("card_backs/classic.dark.png", "png");

        auto const deck = load(dir);

        CHECK(deck.card_backs.empty());
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
        CHECK(deck.card_backs.front().name == "Classic Back");
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
        CHECK(term_for(deck.metadata.origin, "iptc-dst") == "print");
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
        REQUIRE(deck.card_backs.size() == 1);
        CHECK(term_for(deck.card_backs.front().origin, "iptc-dst") == "digitalCreation");
    }

    SECTION("a deck that declares no origin gives its cards none")
    {
        auto const bare = load(make_deck());

        CHECK(bare.metadata.origin.empty());
        CHECK(card_at(bare, "major_arcana.00").origin.empty());
    }
}
