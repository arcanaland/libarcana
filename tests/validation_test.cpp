// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

// The catalogue's invariants.

#include <arcana/deck.hpp>
#include <arcana/validation.hpp>

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <array>
#include <string>
#include <string_view>

using namespace arcana;

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

TEST_CASE("every area is one of the eight", "[validation]")
{
    constexpr std::array<std::string_view, 8> areas{"deck",  "ids",   "images", "backs",
                                                    "cards", "names", "ansi",   "surrogate"};

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


TEST_CASE("exactly one rule needs a whole library", "[validation]")
{
    std::vector<std::string_view> library_wide;
    for (auto const& r : rules())
        if (r.needs == phase::library)
            library_wide.push_back(r.code);

    REQUIRE(library_wide.size() == 1);
    CHECK(library_wide.front() == "duplicate-deck-identifier");
}

TEST_CASE("validate returns nothing", "[validation]")
{
    deck const empty{};
    CHECK(validate(empty).empty());
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
