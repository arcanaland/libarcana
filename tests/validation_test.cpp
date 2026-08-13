// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

// The catalogue's invariants.

#include <arcana/deck.hpp>
#include <arcana/validation.hpp>

#include "validation/fixture.hpp"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>
#include <vector>

using namespace arcana;

using arcana_test::codes_of;
using arcana_test::validate_fixture;
using arcana_test::validation_fixtures;

namespace
{

// [a-z][a-z0-9]*(-[a-z0-9]+)*
bool is_flat_kebab_case(std::string_view text)
{
    if (text.empty() || text.front() < 'a' || text.front() > 'z')
        return false;

    if (text.back() == '-')
        return false;

    bool previous_was_hyphen = false;
    for (char const c : text)
    {
        bool const is_lower = c >= 'a' && c <= 'z';
        bool const is_digit = c >= '0' && c <= '9';
        bool const is_hyphen = c == '-';

        if (!is_lower && !is_digit && !is_hyphen)
            return false;
        if (is_hyphen && previous_was_hyphen)
            return false;

        previous_was_hyphen = is_hyphen;
    }

    return true;
}

std::size_t sentence_count(std::string_view text)
{
    std::size_t count = text.ends_with('.') ? 1 : 0;
    for (std::size_t at = text.find(". "); at != std::string_view::npos;
         at = text.find(". ", at + 1))
        ++count;

    return count;
}

std::size_t word_count(std::string_view text)
{
    return static_cast<std::size_t>(std::ranges::count(text, ' ')) + 1;
}

}  // namespace

TEST_CASE("severity and phase are ordered as declared", "[validation]")
{
    STATIC_REQUIRE(severity::pedantic < severity::info);
    STATIC_REQUIRE(severity::info < severity::warning);
    STATIC_REQUIRE(severity::warning < severity::error);

    STATIC_REQUIRE(phase::document < phase::filesystem);
    STATIC_REQUIRE(phase::filesystem < phase::library);
}

TEST_CASE("schema_range::contains covers its closed interval", "[validation]")
{
    constexpr schema_range two_only{2, 2};
    STATIC_REQUIRE_FALSE(two_only.contains(1));
    STATIC_REQUIRE(two_only.contains(2));
    STATIC_REQUIRE_FALSE(two_only.contains(3));

    constexpr schema_range both{1, 2};
    STATIC_REQUIRE(both.contains(1));
    STATIC_REQUIRE(both.contains(2));
    STATIC_REQUIRE_FALSE(both.contains(0));
}

TEST_CASE("the catalogue is not empty", "[validation]")
{
    CHECK_FALSE(rules().empty());
}

TEST_CASE("the catalogue is sorted strictly ascending by code", "[validation]")
{
    auto const all = rules();
    for (std::size_t i = 1; i < all.size(); ++i) REQUIRE(all[i - 1].code < all[i].code);
}

TEST_CASE("find_rule is a bijection over the catalogue", "[validation]")
{
    for (auto const& r : rules())
    {
        auto const* found = find_rule(r.code);
        REQUIRE(found != nullptr);
        CHECK(found == &r);
    }
}

TEST_CASE("find_rule rejects a code the catalogue does not carry", "[validation]")
{
    CHECK(find_rule("no-such-rule-exists") == nullptr);
    CHECK(find_rule("") == nullptr);
}

TEST_CASE("every rule is fully populated", "[validation]")
{
    for (auto const& r : rules())
    {
        INFO("rule: " << r.code);
        CHECK_FALSE(r.code.empty());
        CHECK_FALSE(r.area.empty());
        CHECK_FALSE(r.spec_ref.empty());
        CHECK_FALSE(r.explanation.empty());
    }
}

TEST_CASE("every code is flat kebab-case", "[validation]")
{
    for (auto const& r : rules())
    {
        INFO("rule: " << r.code);
        CHECK(is_flat_kebab_case(r.code));
    }
}

TEST_CASE("every area is one of the nine", "[validation]")
{
    constexpr std::array<std::string_view, 9> areas{
        "deck", "ids", "images", "backs", "cards", "names", "ansi", "surrogate", "container"
    };

    for (auto const& r : rules())
    {
        INFO("rule: " << r.code);
        CHECK(std::ranges::find(areas, r.area) != areas.end());
    }
}

TEST_CASE("no explanation interpolates", "[validation]")
{
    for (auto const& r : rules())
    {
        INFO("rule: " << r.code);
        CHECK(r.explanation.find('{') == std::string_view::npos);
        CHECK(r.explanation.find('}') == std::string_view::npos);
        CHECK(r.explanation.find('%') == std::string_view::npos);
    }
}

TEST_CASE("every explanation is a sentence or two and no more", "[validation]")
{
    for (auto const& r : rules())
    {
        INFO("rule: " << r.code);
        CHECK(r.explanation.ends_with('.'));
        CHECK(sentence_count(r.explanation) >= 1);
        CHECK(sentence_count(r.explanation) <= 3);
        CHECK(word_count(r.explanation) <= 40);
    }
}

TEST_CASE("every schema range names a major this specification has", "[validation]")
{
    for (auto const& r : rules())
    {
        INFO("rule: " << r.code);
        CHECK(r.applies_to.min >= 1);
        CHECK(r.applies_to.min <= r.applies_to.max);
        CHECK(r.applies_to.max <= 2);
    }
}

TEST_CASE("no rule citing DECK.md#9.4 sits below the specification's floor", "[validation]")
{
    // DECK.md#9.2 defines only `error` and `warning`. `info` and `pedantic`
    // carry findings the specification states no outcome for, so a rule that
    // cites a labelled §9.4 rule cannot be filed as either.
    for (auto const& r : rules())
    {
        INFO("rule: " << r.code);
        if (r.spec_ref.find("#9.4") != std::string_view::npos)
            CHECK(r.default_level >= severity::warning);
    }
}

TEST_CASE("no rule ships experimental", "[validation]")
{
    for (auto const& r : rules())
    {
        INFO("rule: " << r.code);
        CHECK_FALSE(r.experimental);
    }
}


TEST_CASE("every rule beyond a single deck is named", "[validation]")
{
    // `phase::library` carries the rules `validate(deck const&)` structurally
    // cannot reach. One needs sibling decks; the four `container` rules judge a
    // zip a validator is handed before any deck exists (DECK.md#2.4). The phase
    // enum is public API and gained no `container` member for them.
    constexpr std::array<std::string_view, 5> beyond_one_deck{
        "bad-container-entry-type", "bad-container-layout", "duplicate-deck-identifier",
        "missing-container-mimetype", "unsafe-container-entry-name"
    };

    std::vector<std::string_view> library_wide;
    for (auto const& r : rules())
        if (r.needs == phase::library)
            library_wide.push_back(r.code);

    std::ranges::sort(library_wide);
    CHECK(std::ranges::equal(library_wide, beyond_one_deck));
}

TEST_CASE("schema_major", "[validation]")
{
    auto const major_of = [](std::string_view text)
    {
        deck_metadata metadata;
        metadata.schema_version = text;
        return schema_major(metadata);
    };

    CHECK(major_of("2.0") == 2);
    CHECK(major_of("1.0") == 1);
    CHECK(major_of("10.3") == 10);

    CHECK_FALSE(major_of("").has_value());
    CHECK_FALSE(major_of("2").has_value());
    CHECK_FALSE(major_of("2.0.1").has_value());
}

TEST_CASE("schema_major rejects a major it cannot carry", "[validation]")
{
    deck_metadata metadata;
    metadata.schema_version = "256.0";
    CHECK_FALSE(schema_major(metadata).has_value());
}

// ---------------------------------------------------------------------------
// Coverage
// ---------------------------------------------------------------------------

namespace
{

// TODO: need to figure out what to do with these
constexpr std::array<std::string_view, 6> deferred{
    // Needs an image decoder
    "aspect-ratio-mismatch",

    // The only phase::library rule
    "duplicate-deck-identifier",

    // A container is a zip a validator is handed before any deck exists, so
    // validate(deck const&) is never given one (DECK.md#2.4).
    "bad-container-entry-type",
    "bad-container-layout",
    "missing-container-mimetype",
    "unsafe-container-entry-name",
};

// This should be zero when we're done
constexpr std::array<std::string_view, 83> not_yet_covered{
    "artwork-rating-exceeds-deck",
    "backslash-in-path",
    "bad-card-size-mm",
    "bad-content-rating-key",
    "bad-follows",
    "bad-gtin",
    "bad-isbn",
    "bad-language-tag",
    "bad-link-rel",
    "bad-link-url",
    "bad-name-template-placeholder",
    "bad-oars-descriptor",
    "bad-palette-color",
    "bad-palette-snapped-color",
    "bad-pips-value",
    "bad-product-id-key",
    "bad-published-date",
    "bad-rights-field-value",
    "bad-rights-status-uri",
    "bad-schema-version",
    "bad-spdx-expression",
    "bom-in-toml",
    "card-not-baseline-format",
    "card-size-aspect-mismatch",
    "cards-key-path",
    "deck-has-no-cards",
    "deck-rating-exceeds-artwork",
    "declared-card-without-image",
    "deprecated-1-0-key",
    "duplicate-card-position",
    "duplicate-rank-in-ranks",
    "empty-card-number",
    "excluded-card-also-declared",
    "excluded-card-has-image",
    "follows-self",
    "ignored-key-on-variant",
    "language-tag-case-collision",
    "malformed-deck-toml",
    "malformed-name-file",
    "malformed-surrogate-file",
    "missing-alt-text",
    "missing-artwork-complete",
    "missing-deck-toml",
    "missing-default-language-file",
    "missing-license-file",
    "missing-license-text",
    "missing-packager",
    "missing-required-field",
    "missing-variant-image",
    "no-rights-statement",
    "non-canonical-language-tag",
    "non-utf8-name-file",
    "non-utf8-toml",
    "packager-equals-author",
    "palette-snapped-length-mismatch",
    "partial-alt-text-in-facet",
    "position-on-minor-arcanum",
    "rank-without-image",
    "redistribution-contradicts-rights-status",
    "redistribution-narrower-than-license",
    "surrogate-deck-redistribution-full",
    "surrogate-deck-without-buy-link",
    "surrogate-deck-without-license",
    "surrogate-deck-without-signifies",
    "symlink-escapes-deck-root",
    "unknown-artwork-rating-system",
    "unknown-metadata-alt-text-key",
    "unknown-name-entity-kind",
    "unknown-name-facet",
    "unknown-name-key",
    "unknown-surrogate-key",
    "unknown-table",
    "unknown-variant-default",
    "unlocalized-fallback-string",
    "unnamed-extended-major",
    "unregistered-content-rating-system",
    "unregistered-link-rel",
    "unregistered-product-id-scheme",
    "unsafe-path",
    "unused-artwork-complete",
    "variant-card-without-default",
    "variant-for-unknown-card",
    "wrong-value-type",
};

bool contains(std::span<std::string_view const> haystack, std::string_view needle)
{
    return std::ranges::find(haystack, needle) != haystack.end();
}

}  // namespace

TEST_CASE("the three coverage lists partition the catalogue", "[validation][coverage]")
{
    std::vector<std::string_view> covered;
    for (auto const& name : validation_fixtures)
        for (auto const code : codes_of(validate_fixture(name)))
            if (!contains(covered, code))
                covered.push_back(code);

    for (auto const& r : rules())
    {
        INFO("rule: " << r.code);

        int const memberships = static_cast<int>(contains(covered, r.code)) +
                                static_cast<int>(contains(not_yet_covered, r.code)) +
                                static_cast<int>(contains(deferred, r.code));

        CHECK(memberships == 1);
    }

    // Every code a fixture fired is a real one, so the three lists cover the
    // catalogue and nothing else.
    for (auto const code : covered)
    {
        INFO("code: " << code);
        CHECK(find_rule(code) != nullptr);
    }

    CHECK(covered.size() + not_yet_covered.size() + deferred.size() == rules().size());
}

TEST_CASE("no deferred rule has a fixture that fires it", "[validation][coverage]")
{
    for (auto const& name : validation_fixtures)
        for (auto const code : codes_of(validate_fixture(name)))
        {
            INFO("fixture: " << name);
            CHECK_FALSE(contains(deferred, code));
        }
}

// ---------------------------------------------------------------------------
// Harness
// ---------------------------------------------------------------------------

TEST_CASE("validate on a deck with no root on disk reads nothing from a tree", "[validation]")
{
    deck const empty{};

    // walk_deck finds no files, so nothing a check learns here can have come
    // from the filesystem. The document checks still run and see an empty
    // deck.toml, and as layers land more of them have something to say about
    // one, so the assertion is about where a finding came from rather than
    // about how many there are.
    for (auto const& one : validate(empty))
    {
        INFO("code: " << one.code);

        rule const* r = find_rule(one.code);
        REQUIRE(r != nullptr);

        CHECK(r->needs == phase::document);
        CHECK_FALSE(one.path.has_value());
    }
}

TEST_CASE("validate is deterministic", "[validation]")
{
    auto const once = codes_of(validate_fixture(validation_fixtures.front()));
    auto const twice = codes_of(validate_fixture(validation_fixtures.front()));
    CHECK(once == twice);
}

TEST_CASE("validate never runs a rule that needs a whole library", "[validation]")
{
    for (auto const& name : validation_fixtures)
        for (auto const code : codes_of(validate_fixture(name)))
        {
            INFO("code: " << code);
            REQUIRE(find_rule(code) != nullptr);
            CHECK(find_rule(code)->needs != phase::library);
        }
}
