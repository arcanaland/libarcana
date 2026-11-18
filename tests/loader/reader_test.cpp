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
#include <utility>
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

TEST_CASE("every [deck] key is kept as declared", "[loader][v2][metadata]")
{
    using arcana::pip_style;
    using arcana::sharing_terms;

    SECTION("scalar and array keys round-trip")
    {
        auto const deck = load(make_deck(R"(
default_language = "fr"
metadata_language = "en-GB"
artwork_language = ["fr-1694acad", "la"]
packager = "A Packager"
license_files = ["LICENSE", "notices/ART.txt"]
copyright = "© 1760 Nicolas Conver"
rights_status = "http://rightsstatements.org/vocab/NoC-US/1.0/"
published_date = "1760"
)"));

        auto const& metadata = deck.metadata();
        CHECK(metadata.default_language == "fr");
        CHECK(metadata.metadata_language == "en-GB");
        CHECK(metadata.artwork_language == std::vector<std::string>{"fr-1694acad", "la"});
        CHECK(metadata.packager == "A Packager");
        CHECK(metadata.license_files == std::vector<std::string>{"LICENSE", "notices/ART.txt"});
        CHECK(metadata.copyright == "© 1760 Nicolas Conver");
        CHECK(metadata.rights_status == "http://rightsstatements.org/vocab/NoC-US/1.0/");
        CHECK(metadata.published_date == "1760");
    }

    SECTION("an absent key is unstated or empty")
    {
        auto const& metadata = load(make_deck()).metadata();
        CHECK(metadata.pips == pip_style::unstated);
        CHECK(metadata.redistribution == sharing_terms::unstated);
        CHECK(metadata.derivation == sharing_terms::unstated);
        CHECK_FALSE(metadata.default_language.has_value());
        CHECK_FALSE(metadata.metadata_language.has_value());
        CHECK(metadata.artwork_language.empty());
        CHECK_FALSE(metadata.card_size_mm.has_value());
        CHECK_FALSE(metadata.packager.has_value());
        CHECK(metadata.license_files.empty());
        CHECK_FALSE(metadata.copyright.has_value());
        CHECK_FALSE(metadata.rights_status.has_value());
        CHECK_FALSE(metadata.published_date.has_value());
    }

    SECTION("closed vocabularies")
    {
        auto const pips = [](std::string_view value)
        { return load(make_deck(std::format("pips = {}", value))).metadata().pips; };

        CHECK(pips(R"("scenic")") == pip_style::scenic);
        CHECK(pips(R"("emblematic")") == pip_style::emblematic);
        CHECK(pips(R"("unstated")") == pip_style::unstated);
        CHECK(pips(R"("mixed")") == pip_style::unstated);
        CHECK(pips("1") == pip_style::unstated);

        auto const terms = [](std::string_view value)
        {
            auto const deck =
                load(make_deck(std::format("redistribution = {0}\nderivation = {0}", value)));
            CHECK(deck.metadata().redistribution == deck.metadata().derivation);
            return deck.metadata().redistribution;
        };

        CHECK(terms(R"("full")") == sharing_terms::full);
        CHECK(terms(R"("surrogate")") == sharing_terms::surrogate);
        CHECK(terms(R"("none")") == sharing_terms::none);
        CHECK(terms(R"("unstated")") == sharing_terms::unstated);
        CHECK(terms(R"("mixed")") == sharing_terms::unstated);
        CHECK(terms("1") == sharing_terms::unstated);
    }

    SECTION("card_size_mm is exactly two numbers")
    {
        auto const size = [](std::string_view value)
        {
            return load(make_deck(std::format("card_size_mm = {}", value))).metadata().card_size_mm;
        };

        CHECK(size("[70.5, 120.25]") == arcana::card_size{.width = 70.5, .height = 120.25});
        CHECK(size("[70, 120]") == arcana::card_size{.width = 70, .height = 120});
        CHECK_FALSE(size("[70]").has_value());
        CHECK_FALSE(size("[70, 120, 3]").has_value());
        CHECK_FALSE(size(R"("70x120")").has_value());
    }

    SECTION("published_date is a string or nothing")
    {
        auto const date = [](std::string_view value)
        {
            return load(make_deck(std::format("published_date = {}", value)))
                .metadata()
                .published_date;
        };

        CHECK(date(R"("1909-12-01")") == "1909-12-01");
        CHECK_FALSE(date("1909-12-01").has_value());
        CHECK_FALSE(date("1909").has_value());
    }

    SECTION("metadata_language falls back to default_language, then en")
    {
        auto const effective = [](std::string_view manifest)
        {
            return std::string{
                arcana::effective_metadata_language(load(make_deck(manifest)).metadata())
            };
        };

        CHECK(effective("metadata_language = \"de\"\ndefault_language = \"fr\"") == "de");
        CHECK(effective(R"(default_language = "fr")") == "fr");
        CHECK(effective("") == "en");
    }
}

TEST_CASE("links, product ids and [strings] are kept as declared", "[loader][v2][metadata]")
{
    using arcana::deck_link;
    using arcana::link_rel;
    using arcana::product_id;
    using arcana::product_scheme;

    SECTION("a registered rel maps to its enumerator, and any rel keeps its token")
    {
        auto const deck = load(make_deck(R"(links = [
    { rel = "homepage", url = "https://example.com/" },
    { rel = "buy", url = "https://example.com/shop" },
    { rel = "artist", url = "https://example.com/artist" },
    { rel = "publisher", url = "https://example.com/press" },
    { rel = "source", url = "https://example.com/scans" },
    { rel = "mastodon", url = "https://example.social/@deck" },
])"));

        auto const& links = deck.metadata().links;
        REQUIRE(links.size() == 6);
        CHECK(links[0].rel == link_rel::homepage);
        CHECK(links[0].rel_name == "homepage");
        CHECK(links[1].rel == link_rel::buy);
        CHECK(links[1].rel_name == "buy");
        CHECK(links[2].rel == link_rel::artist);
        CHECK(links[2].rel_name == "artist");
        CHECK(links[3].rel == link_rel::publisher);
        CHECK(links[3].rel_name == "publisher");
        CHECK(links[4].rel == link_rel::source);
        CHECK(links[4].rel_name == "source");
        CHECK(links[5].rel == link_rel::other);
        CHECK(links[5].rel_name == "mastodon");
    }

    SECTION("links keep their order and every table entry")
    {
        auto const deck = load(make_deck(R"(links = [
    { rel = "source", url = "https://example.com/scans", title = "The scans" },
    "https://example.com/not-a-table",
    { rel = "buy" },
    { url = "https://example.com/no-rel" },
])"));

        auto const& links = deck.metadata().links;
        REQUIRE(links.size() == 3);
        CHECK(
            links[0] == deck_link{
                            .rel = link_rel::source,
                            .rel_name = "source",
                            .url = "https://example.com/scans",
                            .title = "The scans",
                        }
        );
        CHECK(links[1] == deck_link{.rel = link_rel::buy, .rel_name = "buy", .url = ""});
        CHECK(
            links[2] == deck_link{
                            .rel = link_rel::other,
                            .rel_name = "",
                            .url = "https://example.com/no-rel",
                        }
        );
    }

    SECTION("product ids are verbatim and sorted by scheme")
    {
        auto const deck = load(make_deck(R"(
[deck.product_ids]
publisher_sku = "CBD-78"
isbn = "978-0-13-468599-1"
gtin = "00012345678905"
"com.example.catalog" = "X-1"
upc = ""
)"));

        CHECK(
            deck.metadata().product_ids ==
            std::vector<product_id>{
                {.scheme = product_scheme::other,
                 .scheme_name = "com.example.catalog",
                 .value = "X-1"},
                {.scheme = product_scheme::gtin, .scheme_name = "gtin", .value = "00012345678905"},
                {.scheme = product_scheme::isbn,
                 .scheme_name = "isbn",
                 .value = "978-0-13-468599-1"},
                {.scheme = product_scheme::publisher_sku,
                 .scheme_name = "publisher_sku",
                 .value = "CBD-78"},
            }
        );
    }

    SECTION("[strings]")
    {
        CHECK_FALSE(load(make_deck()).strings().has_value());

        auto const empty = load(make_deck("[strings]")).strings();
        REQUIRE(empty.has_value());
        CHECK(*empty == arcana::string_terms{});

        auto const full = load(make_deck(R"(
[deck.origin]
"iptc-dst" = "print"

[strings]
source = "Transcribed from the faces."
license = "LicenseRef-Booklet"
license_files = ["BOOKLET-LICENSE"]
copyright = "© 2025 Example Press"
rights_status = "http://rightsstatements.org/vocab/InC/1.0/"
attribution = "Names from the Example Press booklet."

[strings.origin]
"iptc-dst" = "humanEdits"
)"))
                              .strings();

        REQUIRE(full.has_value());
        CHECK(full->source == "Transcribed from the faces.");
        CHECK(
            full->origin ==
            std::vector<arcana::origin_term>{{.system = "iptc-dst", .term = "humanEdits"}}
        );
        CHECK(full->license == "LicenseRef-Booklet");
        CHECK(full->license_files == std::vector<std::string>{"BOOKLET-LICENSE"});
        CHECK(full->copyright == "© 2025 Example Press");
        CHECK(full->rights_status == "http://rightsstatements.org/vocab/InC/1.0/");
        CHECK(full->attribution == "Names from the Example Press booklet.");
    }

    SECTION("[strings].origin does not inherit [deck].origin")
    {
        auto const strings = load(make_deck(R"(
[deck.origin]
"iptc-dst" = "print"

[strings]
source = "Transcribed from the faces."
)"))
                                 .strings();

        REQUIRE(strings.has_value());
        CHECK(strings->origin.empty());
    }
}

TEST_CASE("relations are kept as declared", "[loader][v2][metadata]")
{
    using arcana::deck_rel;

    SECTION("a registered rel maps to its enumerator, and any rel keeps its token")
    {
        auto const deck = load(make_deck(R"(related = [
    { rel = "pattern", target = "land.arcana/pattern/rider-waite-smith" },
    { rel = "surrogate_for", target = "com.example/deck/original" },
    { rel = "expands", target = "com.example/deck/base" },
    { rel = "companion", target = "com.example/deck/oracle" },
    { rel = "x-sequel", target = "com.example/deck/sequel" },
])"));

        auto const& related = deck.metadata().related;
        REQUIRE(related.size() == 5);
        CHECK(related[0].rel == deck_rel::pattern);
        CHECK(related[0].rel_name == "pattern");
        CHECK(related[1].rel == deck_rel::surrogate_for);
        CHECK(related[1].rel_name == "surrogate_for");
        CHECK(related[2].rel == deck_rel::expands);
        CHECK(related[2].rel_name == "expands");
        CHECK(related[3].rel == deck_rel::companion);
        CHECK(related[3].rel_name == "companion");
        CHECK(related[4].rel == deck_rel::other);
        CHECK(related[4].rel_name == "x-sequel");
    }

    SECTION("target and title round-trip, and an absent title is nullopt")
    {
        auto const deck = load(make_deck(R"(related = [
    { rel = "companion", target = "com.example/deck/oracle", title = "The Example Oracle" },
    { rel = "companion", target = "com.example/deck/runes" },
])"));

        auto const& related = deck.metadata().related;
        REQUIRE(related.size() == 2);
        CHECK(related[0].target == "com.example/deck/oracle");
        CHECK(related[0].title == "The Example Oracle");
        CHECK(related[1].target == "com.example/deck/runes");
        CHECK_FALSE(related[1].title.has_value());
    }

    SECTION("entries keep array order, shared rels and repeated targets included")
    {
        auto const deck = load(make_deck(R"(related = [
    { rel = "companion", target = "com.example/deck/b" },
    { rel = "companion", target = "com.example/deck/a" },
    { rel = "expands", target = "com.example/deck/a" },
])"));

        auto const& related = deck.metadata().related;
        CHECK(
            related ==
            std::vector<arcana::deck_relation>{
                {.rel = deck_rel::companion,
                 .rel_name = "companion",
                 .target = "com.example/deck/b"},
                {.rel = deck_rel::companion,
                 .rel_name = "companion",
                 .target = "com.example/deck/a"},
                {.rel = deck_rel::expands, .rel_name = "expands", .target = "com.example/deck/a"},
            }
        );
    }

    SECTION("a non-table entry is skipped, and a table is kept whatever it lacks")
    {
        auto const deck = load(make_deck(R"(related = [
    "com.example/deck/bare",
    { target = "com.example/deck/no-rel" },
    { rel = "pattern" },
])"));

        auto const& related = deck.metadata().related;
        REQUIRE(related.size() == 2);
        CHECK(related[0].rel == deck_rel::other);
        CHECK(related[0].rel_name.empty());
        CHECK(related[0].target == "com.example/deck/no-rel");
        CHECK(related[1].rel == deck_rel::pattern);
        CHECK(related[1].target.empty());
    }

    SECTION("an absent or non-array related is empty")
    {
        CHECK(load(make_deck()).metadata().related.empty());
        CHECK(load(make_deck(R"(related = "com.example/deck/x")")).metadata().related.empty());
    }
}

TEST_CASE("only surrogate_for and expands are recognized", "[model][metadata]")
{
    using arcana::deck_rel;
    using arcana::recognition;
    using arcana::relation_recognition;

    CHECK(recognition(deck_rel::surrogate_for) == relation_recognition::recognized);
    CHECK(recognition(deck_rel::expands) == relation_recognition::recognized);
    CHECK(recognition(deck_rel::pattern) == relation_recognition::advisory);
    CHECK(recognition(deck_rel::companion) == relation_recognition::advisory);
    CHECK(recognition(deck_rel::other) == relation_recognition::advisory);
    CHECK(relation_recognition{} == relation_recognition::advisory);
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
        CHECK(lovers.position == 6);
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

TEST_CASE("a standard major's position defaults to its number", "[loader][v2]")
{
    auto const deck = load(make_deck(R"(
[cards."major_arcana.05"]
position = 30
)"));

    for (int i = 0; i <= arcana::max_canonical_major_arcana_number; ++i)
    {
        auto const& c = card_at(deck, std::format("major_arcana.{:02d}", i));
        CHECK(c.position == (i == 5 ? 30 : i));
    }

    CHECK_FALSE(card_at(deck, "minor_arcana.cups.ace").position.has_value());
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

TEST_CASE("a [cards] entry is kept as declared", "[loader][v2][declared]")
{
    SECTION("every field")
    {
        auto dir = make_deck(R"(
[cards."major_arcana.09"]
name = "L'Hermite"
alt_text = "An old man with a lantern."
number = "VIIII"
inscription = "Lux ex tenebris"
artwork_language = ["fr", "la"]
image = "art/hermit.svg"
position = 9
default_variant = "lantern"

[cards."major_arcana.13"]
unnamed = true
unnumbered = true
supplied_name = { text = "La Mort", lang = "fr", source = "booklet" }
)");
        dir.write("art/hermit.svg", "<svg/>");
        dir.write("scalable/major_arcana/09.lantern.svg", "<svg/>");

        auto const deck = load(dir);

        auto const& hermit = card_at(deck, "major_arcana.09").declared;
        CHECK(hermit.name == "L'Hermite");
        CHECK(hermit.alt_text == "An old man with a lantern.");
        CHECK(hermit.number == "VIIII");
        CHECK_FALSE(hermit.unnumbered);
        CHECK_FALSE(hermit.unnamed);
        CHECK_FALSE(hermit.supplied.has_value());
        CHECK(hermit.inscription == std::vector<std::string>{"Lux ex tenebris"});
        CHECK(hermit.artwork_language == std::vector<std::string>{"fr", "la"});
        CHECK(hermit.image == "art/hermit.svg");
        CHECK(hermit.position == 9);
        CHECK(hermit.default_variant == "lantern");

        auto const& death = card_at(deck, "major_arcana.13").declared;
        CHECK(death.unnamed);
        CHECK(death.unnumbered);
        CHECK_FALSE(death.name.has_value());
        CHECK_FALSE(death.number.has_value());
        REQUIRE(death.supplied.has_value());
        CHECK(death.supplied->text == "La Mort");
        CHECK(death.supplied->lang == "fr");
        CHECK(death.supplied->source == arcana::name_source::booklet);
        CHECK(death.supplied->source_name == "booklet");
    }

    SECTION("an inscription array keeps its reading order")
    {
        auto const deck = load(make_deck(R"(
[cards."minor_arcana.wands.ace"]
inscription = ["Sacred Fire", "The Awakening"]
)"));

        CHECK(
            card_at(deck, "minor_arcana.wands.ace").declared.inscription ==
            std::vector<std::string>{"Sacred Fire", "The Awakening"}
        );
    }

    SECTION("a supplied name's source is the registry plus the token as written")
    {
        auto const deck = load(make_deck(R"(
[cards."major_arcana.01"]
supplied_name = { text = "One", source = "packaging" }

[cards."major_arcana.02"]
supplied_name = { text = "Two", source = "publisher" }

[cards."major_arcana.03"]
supplied_name = { text = "Three", source = "packager" }

[cards."major_arcana.04"]
supplied_name = { text = "Four", source = "x_archive" }

[cards."major_arcana.05"]
supplied_name = { text = "Five" }
)"));

        auto const source_of = [&](std::string_view canonical)
        {
            auto const& supplied = card_at(deck, canonical).declared.supplied;
            REQUIRE(supplied.has_value());
            return std::pair{supplied->source, supplied->source_name};
        };

        using arcana::name_source;
        CHECK(
            source_of("major_arcana.01") ==
            std::pair{name_source::packaging, std::string{"packaging"}}
        );
        CHECK(
            source_of("major_arcana.02") ==
            std::pair{name_source::publisher, std::string{"publisher"}}
        );
        CHECK(
            source_of("major_arcana.03") ==
            std::pair{name_source::packager, std::string{"packager"}}
        );
        CHECK(
            source_of("major_arcana.04") == std::pair{name_source::other, std::string{"x_archive"}}
        );
        CHECK(source_of("major_arcana.05") == std::pair{name_source::unstated, std::string{}});
    }

    SECTION("a name file overrides the resolved alt text, not the declared one")
    {
        auto dir = make_deck(R"(
[cards."major_arcana.00"]
alt_text = "Le Mat walks."
)");
        dir.write("names/en.toml", R"(
[alt_text.card.major_arcana]
00 = "The Fool walks."
)");

        auto const& fool = card_at(load(dir), "major_arcana.00");
        CHECK(fool.alt_text == "The Fool walks.");
        CHECK(fool.declared.alt_text == "Le Mat walks.");
    }

    SECTION("a malformed value reads as absent")
    {
        auto const deck = load(make_deck(R"(
[cards."major_arcana.01"]
unnamed = "yes"
unnumbered = 1
supplied_name = "Le Bateleur"
inscription = 7
artwork_language = ["fr", 2]

[cards."major_arcana.02"]
supplied_name = { lang = "fr" }
inscription = ["Juno", 3, "Pope"]
)"));

        auto const& one = card_at(deck, "major_arcana.01").declared;
        CHECK_FALSE(one.unnamed);
        CHECK_FALSE(one.unnumbered);
        CHECK_FALSE(one.supplied.has_value());
        CHECK(one.inscription.empty());
        CHECK(one.artwork_language == std::vector<std::string>{"fr"});

        auto const& two = card_at(deck, "major_arcana.02").declared;
        CHECK_FALSE(two.supplied.has_value());
        CHECK(two.inscription == std::vector<std::string>{"Juno", "Pope"});
    }

    SECTION("a card with no entry declares nothing")
    {
        CHECK(card_at(load(make_deck()), "major_arcana.00").declared == arcana::card_declaration{});
    }
}

TEST_CASE("variant and card back entries are kept as declared", "[loader][v2][declared]")
{
    SECTION("every artwork field on a variant entry")
    {
        auto dir = make_deck(R"(
[cards."major_arcana.06:two_women"]
name = "Les Amoureuses"
alt_text = "Two women beneath an angel."
number = "VI"
unnumbered = true
unnamed = true
supplied_name = { text = "L'Amoureux", lang = "fr", source = "x_archive" }
inscription = ["Amor", "Vincit"]
artwork_language = ["fr"]
image = "extra/two_women.svg"
)");
        dir.write("extra/two_women.svg", "<svg/>");

        auto const deck = load(dir);
        auto const& declared = variant_at(card_at(deck, "major_arcana.06"), "two_women").declared;

        CHECK(declared.name == "Les Amoureuses");
        CHECK(declared.alt_text == "Two women beneath an angel.");
        CHECK(declared.number == "VI");
        CHECK(declared.unnumbered);
        CHECK(declared.unnamed);
        REQUIRE(declared.supplied.has_value());
        CHECK(declared.supplied->text == "L'Amoureux");
        CHECK(declared.supplied->lang == "fr");
        CHECK(declared.supplied->source == arcana::name_source::other);
        CHECK(declared.supplied->source_name == "x_archive");
        CHECK(declared.inscription == std::vector<std::string>{"Amor", "Vincit"});
        CHECK(declared.artwork_language == std::vector<std::string>{"fr"});
        CHECK(declared.image == "extra/two_women.svg");
    }

    SECTION("a variant key's position and default_variant have nowhere to go")
    {
        auto dir = make_deck(R"(
[cards."major_arcana.06"]
position = 6
default_variant = "two_men"

[cards."major_arcana.06:two_women"]
position = 3
default_variant = "two_women"
)");
        dir.write("scalable/major_arcana/06.two_men.svg", "<svg/>");
        dir.write("scalable/major_arcana/06.two_women.svg", "<svg/>");

        auto const deck = load(dir);
        auto const& lovers = card_at(deck, "major_arcana.06");

        CHECK(variant_at(lovers, "two_women").declared == arcana::variant_declaration{});
        CHECK(lovers.declared.position == 6);
        CHECK(lovers.declared.default_variant == "two_men");
        CHECK(lovers.position == 6);
        CHECK(lovers.default_variant == "two_men");
    }

    SECTION("every field on a back")
    {
        auto dir = make_deck(R"(
[card_backs.designs.classic]
name = "Classic"
alt_text = "A blue lattice."
description = "The house back."
inscription = "Arcana"
artwork_language = ["la"]
reversible = true
image = "backs/classic.png"

[card_backs.designs.ornate]
inscription = ["Ave", "Vale"]
image = "backs/ornate.png"
)");
        dir.write("backs/classic.png", "png");
        dir.write("backs/ornate.png", "png");

        auto const deck = load(dir);
        REQUIRE(deck.card_backs().size() == 2);

        auto const& classic = deck.card_backs()[0];
        REQUIRE(classic.id == "classic");
        CHECK(classic.declaration.name == "Classic");
        CHECK(classic.declaration.alt_text == "A blue lattice.");
        CHECK(classic.declaration.inscription == std::vector<std::string>{"Arcana"});
        CHECK(classic.declaration.artwork_language == std::vector<std::string>{"la"});
        CHECK(classic.declaration.reversible == true);
        CHECK(classic.declaration.image == "backs/classic.png");
        CHECK(classic.description == "The house back.");
        CHECK(classic.image_ref == "backs/classic.png");

        auto const& ornate = deck.card_backs()[1];
        REQUIRE(ornate.id == "ornate");
        CHECK(ornate.declaration.inscription == std::vector<std::string>{"Ave", "Vale"});
    }

    SECTION("reversible has three states, and a non-bool is the third")
    {
        auto dir = make_deck(R"(
[card_backs.designs.a]
reversible = true

[card_backs.designs.b]
reversible = false

[card_backs.designs.c]

[card_backs.designs.d]
reversible = 1
)");
        for (auto const* key : {"a", "b", "c", "d"})
            dir.write(std::format("card_backs/{}.png", key), "png");

        auto const deck = load(dir);
        REQUIRE(deck.card_backs().size() == 4);

        CHECK(deck.card_backs()[0].declaration.reversible == true);
        CHECK(deck.card_backs()[1].declaration.reversible == false);
        CHECK_FALSE(deck.card_backs()[2].declaration.reversible.has_value());
        CHECK_FALSE(deck.card_backs()[3].declaration.reversible.has_value());
    }

    SECTION("a name file overrides the resolved back strings, not the declared ones")
    {
        auto dir = make_deck(R"(
[card_backs.designs.classic]
name = "Dos classique"
alt_text = "Un treillis bleu."
)");
        dir.write("card_backs/classic.png", "png");
        dir.write("names/en.toml", R"(
[name.card_back]
classic = "Classic Back"

[alt_text.card_back]
classic = "A blue lattice."
)");

        auto const deck = load(dir);
        REQUIRE(deck.card_backs().size() == 1);

        auto const& classic = deck.card_backs().front();
        CHECK(classic.name == "Classic Back");
        CHECK(classic.alt_text == "A blue lattice.");
        CHECK(classic.declaration.name == "Dos classique");
        CHECK(classic.declaration.alt_text == "Un treillis bleu.");
    }

    SECTION("a back known only from discovery declares nothing")
    {
        auto dir = make_deck();
        dir.write("card_backs/classic.png", "png");

        auto const deck = load(dir);
        REQUIRE(deck.card_backs().size() == 1);

        CHECK(deck.card_backs().front().declaration == arcana::card_back_declaration{});
        CHECK_FALSE(deck.card_backs().front().declared);
    }
}

TEST_CASE("card names resolve by rc.1's chain", "[loader][v2][names]")
{
    // DECK.md#73-display-name-resolution

    SECTION("a supplied name names a card nothing else names")
    {
        auto const deck = load(make_deck(R"(
[cards."major_arcana.13"]
unnamed = true
supplied_name = { text = "La Mort", source = "booklet" }

[cards."minor_arcana.cups.ace"]
supplied_name = { text = "As de Coupe" }
)"));

        CHECK(card_at(deck, "major_arcana.13").display_name == "La Mort");
        CHECK(card_at(deck, "minor_arcana.cups.ace").display_name == "As de Coupe");
    }

    SECTION("a name file outranks a supplied name")
    {
        auto dir = make_deck(R"(
[cards."major_arcana.13"]
supplied_name = { text = "La Mort" }
)");
        dir.write("names/en.toml", R"(
[name.card.major_arcana]
13 = "Death"
)");

        CHECK(card_at(load(dir), "major_arcana.13").display_name == "Death");
    }

    SECTION("name beats supplied_name on the same entry")
    {
        auto const deck = load(make_deck(R"(
[cards."major_arcana.13"]
name = "L'Arcane sans nom"
supplied_name = { text = "La Mort" }
)"));

        CHECK(card_at(deck, "major_arcana.13").display_name == "L'Arcane sans nom");
    }

    SECTION("an unnamed card has no name")
    {
        auto dir = make_deck(R"(
[cards."major_arcana.13"]
unnamed = true

[cards."major_arcana.squirrel"]
unnamed = true

[cards."minor_arcana.cups.ace"]
unnamed = true
)");
        dir.write("scalable/major_arcana/squirrel.svg", "<svg/>");

        auto const deck = load(dir);

        CHECK(card_at(deck, "major_arcana.13").display_name.empty());
        CHECK(card_at(deck, "major_arcana.squirrel").display_name.empty());

        auto const& ace = card_at(deck, "minor_arcana.cups.ace");
        CHECK(ace.display_name.empty());
        CHECK(ace.display_suit == "Cups");
        CHECK(ace.display_rank == "Ace");
    }

    SECTION("a name file still names an unnamed card")
    {
        auto dir = make_deck(R"(
[cards."major_arcana.13"]
unnamed = true
)");
        dir.write("names/en.toml", R"(
[name.card.major_arcana]
13 = "Death"
)");

        CHECK(card_at(load(dir), "major_arcana.13").display_name == "Death");
    }

    SECTION("an extended major nothing names has no name")
    {
        auto dir = make_deck(R"(
[cards."major_arcana.23"]
number = "XXIII"
)");
        dir.write("scalable/major_arcana/23.svg", "<svg/>");
        dir.write("scalable/major_arcana/24.svg", "<svg/>");
        dir.write("names/en.toml", R"(
[name.card.major_arcana]
24 = "The Twenty-Fourth"
)");

        auto const deck = load(dir);

        CHECK(card_at(deck, "major_arcana.23").display_name.empty());
        CHECK(card_at(deck, "major_arcana.24").display_name == "The Twenty-Fourth");
    }

    SECTION("a custom major nothing names is still its title-cased key")
    {
        auto dir = make_deck();
        dir.write("scalable/major_arcana/happy_squirrel.svg", "<svg/>");

        CHECK(card_at(load(dir), "major_arcana.happy_squirrel").display_name == "Happy Squirrel");
    }

    SECTION("a variant that declares nothing inherits its card's name, empty or supplied")
    {
        auto dir = make_deck(R"(
[cards."major_arcana.13"]
unnamed = true

[cards."major_arcana.12"]
unnamed = true
supplied_name = { text = "Le Pendu" }
)");
        dir.write("scalable/major_arcana/13.svg", "<svg/>");
        dir.write("scalable/major_arcana/13.time.svg", "<svg/>");
        dir.write("scalable/major_arcana/12.svg", "<svg/>");
        dir.write("scalable/major_arcana/12.alt.svg", "<svg/>");

        auto const deck = load(dir);

        CHECK(variant_at(card_at(deck, "major_arcana.13"), "time").display_name.empty());
        CHECK(variant_at(card_at(deck, "major_arcana.12"), "alt").display_name == "Le Pendu");
    }
}

TEST_CASE("variant names resolve by rc.1's chain", "[loader][v2][names][variants]")
{
    auto const lovers_with = [](std::string_view manifest, std::string_view names = "")
    {
        auto dir = make_deck(manifest);
        dir.write("scalable/major_arcana/06.svg", "<svg/>");
        dir.write("scalable/major_arcana/06.two_women.svg", "<svg/>");
        if (!names.empty())
            dir.write("names/en.toml", names);

        auto const deck = load(dir);
        return variant_at(card_at(deck, "major_arcana.06"), "two_women");
    };

    SECTION("a supplied name names a variant nothing else names")
    {
        auto const manifest = R"(
[cards."major_arcana.06:two_women"]
supplied_name = { text = "Les Amoureuses" }
)";

        CHECK(lovers_with(manifest).display_name == "Les Amoureuses");

        auto const named = lovers_with(manifest, R"(
[name.variant]
"major_arcana.06:two_women" = "The Lovers, Two Women"
)");
        CHECK(named.display_name == "The Lovers, Two Women");
    }

    SECTION("a variant's name beats its own supplied name")
    {
        auto const variant = lovers_with(R"(
[cards."major_arcana.06:two_women"]
name = "Deux Femmes"
supplied_name = { text = "Les Amoureuses" }
)");

        CHECK(variant.display_name == "Deux Femmes");
    }

    SECTION("an unnamed variant of a named card has no name")
    {
        auto const manifest = R"(
[cards."major_arcana.06:two_women"]
unnamed = true
)";

        auto const unnamed = lovers_with(manifest);
        CHECK(unnamed.display_name.empty());
        CHECK(unnamed.alt_text == std::nullopt);

        auto const named = lovers_with(manifest, R"(
[name.variant]
"major_arcana.06:two_women" = "The Lovers, Two Women"
)");
        CHECK(named.display_name == "The Lovers, Two Women");
    }

    SECTION("an unnamed variant with a supplied name takes the supplied text")
    {
        auto const variant = lovers_with(R"(
[cards."major_arcana.06:two_women"]
unnamed = true
supplied_name = { text = "Les Amoureuses", source = "booklet" }
)");

        CHECK(variant.display_name == "Les Amoureuses");
    }

    SECTION("a card's unnamed does not reach a variant that names itself")
    {
        auto const variant = lovers_with(R"(
[cards."major_arcana.06"]
unnamed = true

[cards."major_arcana.06:two_women"]
name = "Deux Femmes"
)");

        CHECK(variant.display_name == "Deux Femmes");
    }

    SECTION("a variant that declares nothing still inherits its card's strings")
    {
        auto const variant = lovers_with(R"(
[cards."major_arcana.06"]
alt_text = "Two figures beneath an angel."
)");

        CHECK(variant.display_name == "The Lovers");
        CHECK(variant.alt_text == "Two figures beneath an angel.");
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

[card_backs.variants.classic]
name = "Classic"
alt_text = "A blue lattice."
image = "card_backs/classic.png"
)");
    dir.write("card_backs/classic.png", "png");

    auto const deck = load(dir);

    auto const& metadata = deck.metadata();
    CHECK(metadata.pips == arcana::pip_style::unstated);
    CHECK_FALSE(metadata.default_language.has_value());
    CHECK_FALSE(metadata.metadata_language.has_value());
    CHECK(metadata.artwork_language.empty());
    CHECK_FALSE(metadata.card_size_mm.has_value());
    CHECK_FALSE(metadata.packager.has_value());
    CHECK(metadata.license_files.empty());
    CHECK_FALSE(metadata.copyright.has_value());
    CHECK_FALSE(metadata.rights_status.has_value());
    CHECK(metadata.redistribution == arcana::sharing_terms::unstated);
    CHECK(metadata.derivation == arcana::sharing_terms::unstated);
    CHECK_FALSE(metadata.published_date.has_value());
    CHECK(arcana::effective_metadata_language(metadata) == "en");
    CHECK(metadata.product_ids.empty());
    CHECK(metadata.related.empty());
    CHECK(metadata.links.empty());
    CHECK(metadata.origin.empty());
    CHECK_FALSE(metadata.identifier.has_value());
    CHECK_FALSE(deck.strings().has_value());

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

    REQUIRE_FALSE(deck.cards().empty());
    for (auto const& c : deck.cards())
    {
        CHECK(c.declared == arcana::card_declaration{});
        CHECK(c.origin.empty());
        for (auto const& variant : c.variants)
            CHECK(variant.declared == arcana::variant_declaration{});
    }

    REQUIRE(deck.card_backs().size() == 1);
    CHECK(deck.card_backs().front().declared);
    CHECK(deck.card_backs().front().declaration == arcana::card_back_declaration{});
    CHECK(deck.card_backs().front().origin.empty());
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
