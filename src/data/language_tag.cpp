// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include "language_tag.hpp"

#include "ascii.hpp"
#include "tables.hpp"
#include "text.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace arcana::data
{

namespace
{

constexpr std::size_t no_subtag = std::string_view::npos;

constexpr auto grandfathered = std::to_array<std::string_view>(
    {"art-lojban", "cel-gaulish", "en-GB-oed", "i-ami",      "i-bnn",     "i-default", "i-enochian",
     "i-hak",      "i-klingon",   "i-lux",     "i-mingo",    "i-navajo",  "i-pwn",     "i-tao",
     "i-tay",      "i-tsu",       "no-bok",    "no-nyn",     "sgn-BE-FR", "sgn-BE-NL", "sgn-CH-DE",
     "zh-guoyu",   "zh-hakka",    "zh-min",    "zh-min-nan", "zh-xiang"}
);

bool is_grandfathered_tag(std::string_view tag) noexcept
{
    return std::ranges::any_of(
        grandfathered, [tag](std::string_view known) { return equal_ignoring_case(tag, known); }
    );
}

// Every RFC 5646 subtag is 1 to 8 characters.
constexpr std::size_t max_subtag_length = 8;

// A registered language subtag is 5 to 8 letters; 4 letters is reserved.
constexpr std::size_t min_registered_language = 5;

// A variant is 5*8alphanum, or 4 characters starting with a digit.
constexpr std::size_t min_variant_length = 5;

// Splits a tag on "-".
//
// @returns Empty where any subtag is not 1 to 8 alphanumerics, which is the one
//          constraint every RFC 5646 production shares.
std::vector<std::string_view> split_subtags(std::string_view tag)
{
    auto subtags = pieces(tag, '-');

    auto const well_formed = [](std::string_view piece)
    {
        return !piece.empty() && piece.size() <= max_subtag_length &&
               std::ranges::all_of(piece, is_alnum);
    };

    if (!std::ranges::all_of(subtags, well_formed))
        return {};

    return subtags | std::ranges::to<std::vector>();
}

bool is_all_alpha(std::string_view subtag) noexcept
{
    return std::ranges::all_of(subtag, is_alpha);
}

bool is_script_subtag(std::string_view subtag) noexcept
{
    return subtag.size() == 4 && is_all_alpha(subtag);
}

bool is_region_subtag(std::string_view subtag) noexcept
{
    return (subtag.size() == 2 && is_all_alpha(subtag)) ||
           (subtag.size() == 3 && std::ranges::all_of(subtag, is_digit));
}

// variant = 5*8alphanum / (DIGIT 3alphanum). Every subtag here is already known
// to be 1 to 8 alphanumerics.
bool is_variant_subtag(std::string_view subtag) noexcept
{
    return subtag.size() >= min_variant_length || (subtag.size() == 4 && is_digit(subtag.front()));
}

bool is_singleton(std::string_view subtag) noexcept
{
    return subtag.size() == 1 && is_alnum(subtag.front()) && to_lower(subtag.front()) != 'x';
}

bool is_private_singleton(std::string_view subtag) noexcept
{
    return subtag.size() == 1 && to_lower(subtag.front()) == 'x';
}

// Consumes the language production and any extlang subtags after it.
//
// @returns The number of subtags taken, or 0 where the first is not a language.
std::size_t take_language(std::span<std::string_view const> subtags) noexcept
{
    constexpr std::size_t max_extlang = 3;

    if (subtags.empty() || !is_all_alpha(subtags.front()))
        return 0;

    auto const size = subtags.front().size();
    if (size == 4 || (size >= min_registered_language && size <= max_subtag_length))
        return 1;

    if (size < 2 || size > 3)
        return 0;

    // extlang is unambiguous here: no production that may follow a language
    // admits a three-letter alphabetic subtag.
    std::size_t taken = 1;
    while (taken <= max_extlang && taken < subtags.size() && subtags[taken].size() == 3 &&
           is_all_alpha(subtags[taken]))
        ++taken;

    return taken;
}

// Where the optional single-occurrence subtags landed.
struct langtag_shape
{
    bool well_formed = false;
    std::size_t script = no_subtag;
    std::size_t region = no_subtag;
};

// Consumes "singleton 1*("-" 2*8alphanum)" runs starting at `at`.
//
// @returns The index after the last one, or `no_subtag` where a singleton is
//          not followed by at least one subtag of its own.
std::size_t take_extensions(std::span<std::string_view const> subtags, std::size_t from) noexcept
{
    while (from < subtags.size() && is_singleton(subtags[from]))
    {
        ++from;

        std::size_t const first = from;
        while (from < subtags.size() && subtags[from].size() >= 2) ++from;

        if (from == first)
            return no_subtag;
    }

    return from;
}

langtag_shape analyze_langtag(std::span<std::string_view const> subtags) noexcept
{
    langtag_shape shape;

    std::size_t next = take_language(subtags);
    if (next == 0)
        return shape;

    if (next < subtags.size() && is_script_subtag(subtags[next]))
        shape.script = next++;

    if (next < subtags.size() && is_region_subtag(subtags[next]))
        shape.region = next++;

    while (next < subtags.size() && is_variant_subtag(subtags[next])) ++next;

    next = take_extensions(subtags, next);
    if (next == no_subtag)
        return shape;

    // Nothing may follow a private use sequence, so it takes the rest.
    if (next < subtags.size() && is_private_singleton(subtags[next]))
        next = (next + 1 == subtags.size()) ? no_subtag : subtags.size();

    shape.well_formed = next == subtags.size();
    return shape;
}

template <typename Transform>
std::string map_chars(std::string_view subtag, Transform transform)
{
    std::string out;
    out.reserve(subtag.size());
    for (char const c : subtag) out.push_back(transform(c));

    return out;
}

std::string canonical_subtag(std::string_view subtag, std::size_t index, langtag_shape const& shape)
{
    if (index == shape.script)
    {
        std::string titlecased = map_chars(subtag, to_lower);
        titlecased.front() = to_upper(titlecased.front());
        return titlecased;
    }

    // A three-digit region is left alone by an upper-casing pass.
    if (index == shape.region)
        return map_chars(subtag, to_upper);

    std::string lowered = map_chars(subtag, to_lower);
    if (index != 0)
        return lowered;

    auto const shortest = shortest_language_subtag(lowered);
    return shortest.has_value() ? std::string{*shortest} : lowered;
}

std::string join_subtags(std::span<std::string_view const> subtags, langtag_shape const& shape)
{
    std::string out;

    for (std::size_t index = 0; index < subtags.size(); ++index)
    {
        if (index != 0)
            out.push_back('-');

        out += canonical_subtag(subtags[index], index, shape);
    }

    return out;
}

}  // namespace

bool is_well_formed_language_tag(std::string_view tag)
{
    if (is_grandfathered_tag(tag))
        return true;

    auto const subtags = split_subtags(tag);
    if (subtags.empty())
        return false;

    // privateuse = "x" 1*("-" 1*8alphanum), which may stand as a whole tag.
    if (is_private_singleton(subtags.front()))
        return subtags.size() >= 2;

    return analyze_langtag(subtags).well_formed;
}

std::string canonicalize_language_tag(std::string_view tag)
{
    // Rewriting one of these to its preferred value needs the IANA registry,
    // which does not ship here.
    if (is_grandfathered_tag(tag))
        return std::string{tag};

    auto const subtags = split_subtags(tag);
    if (subtags.empty())
        return {};

    if (is_private_singleton(subtags.front()))
        return subtags.size() >= 2 ? join_subtags(subtags, {}) : std::string{};

    auto const shape = analyze_langtag(subtags);
    if (!shape.well_formed)
        return {};

    return join_subtags(subtags, shape);
}

bool is_canonical_language_tag(std::string_view tag)
{
    return is_well_formed_language_tag(tag) && canonicalize_language_tag(tag) == tag;
}

}  // namespace arcana::data
