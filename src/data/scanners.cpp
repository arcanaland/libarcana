// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include "scanners.hpp"

#include "spdx_licenses.hpp"
#include "tables.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace arcana::data
{

namespace
{

constexpr bool is_lcalpha(char c) noexcept
{
    return c >= 'a' && c <= 'z';
}

constexpr bool is_digit(char c) noexcept
{
    return c >= '0' && c <= '9';
}

constexpr bool is_alpha(char c) noexcept
{
    return is_lcalpha(c) || (c >= 'A' && c <= 'Z');
}

constexpr bool is_alnum(char c) noexcept
{
    return is_alpha(c) || is_digit(c);
}

constexpr char to_lower(char c) noexcept
{
    return (c >= 'A' && c <= 'Z') ? static_cast<char>(c - 'A' + 'a') : c;
}

constexpr char to_upper(char c) noexcept
{
    return is_lcalpha(c) ? static_cast<char>(c - 'a' + 'A') : c;
}

bool equal_ignoring_case(std::string_view left, std::string_view right) noexcept
{
    return std::ranges::equal(
        left, right, [](char one, char two) { return to_lower(one) == to_lower(two); }
    );
}

// Splits `s` on `sep` and requires every piece to satisfy `piece_ok`.
//
// @returns The number of pieces, or 0 where any piece is empty or rejected. An
//          empty `s` is one empty piece and therefore also 0.
template <typename Predicate>
std::size_t count_pieces(std::string_view s, char sep, Predicate piece_ok) noexcept
{
    std::size_t pieces = 0;

    while (true)
    {
        auto const sep_at = s.find(sep);
        auto const piece = s.substr(0, sep_at);
        if (!piece_ok(piece))
            return 0;

        ++pieces;
        if (sep_at == std::string_view::npos)
            return pieces;

        s.remove_prefix(sep_at + 1);
    }
}

}  // namespace

// --- Identifiers ----------------------------------------------------------

bool is_custom_name(std::string_view s) noexcept
{
    if (s.empty())
        return false;

    if (!is_lcalpha(s.front()) && s.front() != '_')
        return false;

    return std::ranges::all_of(s, [](char c) { return is_lcalpha(c) || is_digit(c) || c == '_'; });
}

bool is_reserved_canonical_key(std::string_view s) noexcept
{
    // DECK.md section 3.2: the two arcana, the four canonical suits and the
    // fourteen canonical ranks.
    constexpr auto reserved = std::to_array<std::string_view>(
        {"ace",          "cups",         "eight", "five",  "four",      "king",  "knight",
         "major_arcana", "minor_arcana", "nine",  "page",  "pentacles", "queen", "seven",
         "six",          "swords",       "ten",   "three", "two",       "wands"}
    );

    static_assert(std::ranges::is_sorted(reserved), "binary_search needs a sorted table");

    return std::ranges::binary_search(reserved, s);
}

namespace
{

// major-key = canonical-major / custom-name, where canonical-major is 2DIGIT.
bool is_major_key(std::string_view key) noexcept
{
    if (key.size() == 2 && is_digit(key.front()) && is_digit(key.back()))
        return true;

    return is_custom_name(key);
}

}  // namespace

bool is_canonical_id(std::string_view s) noexcept
{
    constexpr std::string_view major_prefix{"major_arcana."};
    constexpr std::string_view minor_prefix{"minor_arcana."};

    if (s.starts_with(major_prefix))
        return is_major_key(s.substr(major_prefix.size()));

    if (!s.starts_with(minor_prefix))
        return false;

    // suit-key and rank-key are both custom-name productions, and a custom name
    // cannot contain a dot, so there is exactly one dot left to split on.
    auto const rest = s.substr(minor_prefix.size());
    auto const dot = rest.find('.');
    if (dot == std::string_view::npos)
        return false;

    return is_custom_name(rest.substr(0, dot)) && is_custom_name(rest.substr(dot + 1));
}

bool is_card_reference(std::string_view s) noexcept
{
    auto const colon = s.find(':');
    if (colon == std::string_view::npos)
        return is_canonical_id(s);

    return is_variant_reference(s);
}

bool is_variant_reference(std::string_view s) noexcept
{
    auto const colon = s.find(':');
    if (colon == std::string_view::npos)
        return false;

    return is_canonical_id(s.substr(0, colon)) && is_custom_name(s.substr(colon + 1));
}

namespace
{

// DECK.md section 3.5: label = lcalpha [ *61( lcalpha / DIGIT / "-" ) ( lcalpha / DIGIT ) ]
bool is_label(std::string_view label) noexcept
{
    constexpr std::size_t max_label = 63;

    if (label.empty() || label.size() > max_label || !is_lcalpha(label.front()))
        return false;

    if (label.size() == 1)
        return true;

    if (!is_lcalpha(label.back()) && !is_digit(label.back()))
        return false;

    auto const middle = label.substr(1, label.size() - 2);
    return std::ranges::all_of(
        middle, [](char c) { return is_lcalpha(c) || is_digit(c) || c == '-'; }
    );
}

bool is_path_segment(std::string_view segment) noexcept
{
    return !segment.empty() &&
           std::ranges::all_of(
               segment, [](char c) { return is_lcalpha(c) || is_digit(c) || c == '-'; }
           );
}

bool is_fragment(std::string_view fragment) noexcept
{
    return !fragment.empty() && std::ranges::all_of(
                                    fragment,
                                    [](char c)
                                    {
                                        return is_lcalpha(c) || is_digit(c) || c == '.' ||
                                               c == '_' || c == '-' || c == ':';
                                    }
                                );
}

}  // namespace

bool is_realm(std::string_view s) noexcept
{
    // A realm has two labels or more; a single bare label is not a realm.
    return count_pieces(s, '.', is_label) >= 2;
}

std::optional<qualified_identifier> parse_qualified_identifier(std::string_view s) noexcept
{
    qualified_identifier parts;

    // A fragment cannot contain a hash and no other production admits one, so
    // the first hash is the separator.
    if (auto const hash = s.find('#'); hash != std::string_view::npos)
    {
        parts.fragment = s.substr(hash + 1);
        if (!is_fragment(parts.fragment))
            return std::nullopt;

        s = s.substr(0, hash);
    }

    // The realm ends at the first slash.
    auto const slash = s.find('/');
    if (slash == std::string_view::npos)
        return std::nullopt;

    parts.realm = s.substr(0, slash);
    parts.path = s.substr(slash + 1);

    if (!is_realm(parts.realm) || count_pieces(parts.path, '/', is_path_segment) == 0)
        return std::nullopt;

    return parts;
}

bool is_qualified_identifier(std::string_view s) noexcept
{
    return parse_qualified_identifier(s).has_value();
}

// --- Language tags --------------------------------------------------------

namespace
{

constexpr std::size_t no_subtag = std::string_view::npos;

// RFC 5646 section 2.1's grandfathered production, irregular and regular both.
// Some of these also parse as a langtag; listing them all costs nothing and
// keeps the set in one place.
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
    std::vector<std::string_view> subtags;

    while (true)
    {
        auto const dash = tag.find('-');
        auto const piece = tag.substr(0, dash);
        if (piece.empty() || piece.size() > max_subtag_length ||
            !std::ranges::all_of(piece, is_alnum))
            return {};

        subtags.push_back(piece);
        if (dash == std::string_view::npos)
            return subtags;

        tag.remove_prefix(dash + 1);
    }
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

// --- Colours and URLs -----------------------------------------------------

bool is_srgb_hex_triplet(std::string_view s) noexcept
{
    constexpr std::size_t triplet_size = 7;

    return s.size() == triplet_size && s.front() == '#' &&
           std::ranges::all_of(
               s.substr(1), [](char c) { return is_digit(c) || (c >= 'a' && c <= 'f'); }
           );
}

bool is_absolute_http_url(std::string_view s) noexcept
{
    // RFC 3986 reserves the scheme case-insensitively.
    constexpr std::string_view plain{"http://"};
    constexpr std::string_view secure{"https://"};

    std::string_view rest = s;
    if (equal_ignoring_case(s.substr(0, plain.size()), plain))
        rest.remove_prefix(plain.size());
    else if (equal_ignoring_case(s.substr(0, secure.size()), secure))
        rest.remove_prefix(secure.size());
    else
        return false;

    // A URL with no authority has no host to reach.
    if (rest.empty() || rest.front() == '/' || rest.front() == '?' || rest.front() == '#')
        return false;

    // The characters RFC 3986 excludes outright, plus everything outside
    // printable ASCII. Nothing here is fetched, so this goes no further.
    return std::ranges::all_of(
        s,
        [](char c)
        {
            constexpr std::string_view excluded{"<>\"{}|\\^`"};
            constexpr unsigned char del = 0x7F;

            auto const byte = static_cast<unsigned char>(c);
            return byte > ' ' && byte < del && !excluded.contains(c);
        }
    );
}

// --- SPDX expressions -----------------------------------------------------

namespace
{

constexpr std::string_view license_ref_prefix{"LicenseRef-"};
constexpr std::string_view document_ref_prefix{"DocumentRef-"};

bool is_spdx_operator(std::string_view token) noexcept
{
    return token == "AND" || token == "OR" || token == "WITH";
}

// idstring = 1*(ALPHA / DIGIT / "-" / ".")
bool is_idstring(std::string_view token) noexcept
{
    return !token.empty() &&
           std::ranges::all_of(token, [](char c) { return is_alnum(c) || c == '-' || c == '.'; });
}

// license-ref = ["DocumentRef-" idstring ":"] "LicenseRef-" idstring
//
// Well-formed by construction: a LicenseRef names terms with no SPDX identifier
// and is never looked up on the list.
bool is_license_ref(std::string_view token) noexcept
{
    if (token.starts_with(document_ref_prefix))
    {
        auto const colon = token.find(':');
        if (colon == std::string_view::npos)
            return false;

        auto const document =
            token.substr(document_ref_prefix.size(), colon - document_ref_prefix.size());
        if (!is_idstring(document))
            return false;

        token = token.substr(colon + 1);
    }

    if (!token.starts_with(license_ref_prefix))
        return false;

    return is_idstring(token.substr(license_ref_prefix.size()));
}

std::vector<std::string_view> tokenize_spdx(std::string_view text)
{
    std::vector<std::string_view> tokens;
    std::size_t pos = 0;

    auto const is_space = [](char c) { return static_cast<unsigned char>(c) <= ' '; };
    auto const is_break = [is_space](char c) { return c == '(' || c == ')' || is_space(c); };

    while (pos < text.size())
    {
        if (is_space(text[pos]))
        {
            ++pos;
            continue;
        }

        std::size_t const first = pos;
        if (text[pos] == '(' || text[pos] == ')')
            ++pos;
        else
            while (pos < text.size() && !is_break(text[pos])) ++pos;

        tokens.push_back(text.substr(first, pos - first));
    }

    return tokens;
}

// Recursive descent over the SPDX license expression grammar, SPDX 2.3 Annex D.
//
// Precedence, loosest first: OR, AND, WITH. The "+" of an "or later" identifier
// binds tightest of all and is part of the token.
class spdx_parser
{
  public:
    explicit spdx_parser(std::string_view text) : tokens_(tokenize_spdx(text)) {}

    [[nodiscard]] spdx_expression_check run()
    {
        if (tokens_.empty() || !parse_or() || at_ != tokens_.size())
            return {.well_formed = false, .unknown_identifier = {}};

        return {.well_formed = true, .unknown_identifier = unknown_};
    }

  private:
    [[nodiscard]] std::string_view peek() const noexcept
    {
        return at_ < tokens_.size() ? tokens_[at_] : std::string_view{};
    }

    void note_unknown(std::string_view id) noexcept
    {
        if (unknown_.empty())
            unknown_ = id;
    }

    bool parse_or()
    {
        if (!parse_and())
            return false;

        while (peek() == "OR")
        {
            ++at_;
            if (!parse_and())
                return false;
        }

        return true;
    }

    bool parse_and()
    {
        if (!parse_with())
            return false;

        while (peek() == "AND")
        {
            ++at_;
            if (!parse_with())
                return false;
        }

        return true;
    }

    bool parse_with()
    {
        if (!parse_primary())
            return false;

        if (peek() != "WITH")
            return true;

        ++at_;
        auto const exception = peek();
        if (!is_idstring(exception) || is_spdx_operator(exception))
            return false;

        ++at_;
        if (!is_spdx_exception_id(exception))
            note_unknown(exception);

        return true;
    }

    bool parse_primary()
    {
        if (peek() != "(")
            return parse_simple();

        ++at_;
        if (!parse_or() || peek() != ")")
            return false;

        ++at_;
        return true;
    }

    bool parse_simple()
    {
        auto const token = peek();
        if (token.empty() || token == ")" || is_spdx_operator(token))
            return false;

        ++at_;

        if (token.starts_with(license_ref_prefix) || token.starts_with(document_ref_prefix))
            return is_license_ref(token);

        // license-id "+" is the "or later" form; the plus is not an operator.
        auto id = token;
        if (id.ends_with('+'))
            id.remove_suffix(1);

        if (!is_idstring(id))
            return false;

        if (!is_spdx_license_id(id))
            note_unknown(id);

        return true;
    }

    std::vector<std::string_view> tokens_;
    std::size_t at_ = 0;
    std::string_view unknown_;
};

}  // namespace

spdx_expression_check check_spdx_expression(std::string_view s)
{
    return spdx_parser{s}.run();
}

// --- Image formats --------------------------------------------------------

image_format sniff_image_format(std::span<std::byte const> head) noexcept
{
    constexpr std::array png_signature{
        std::byte{0x89}, std::byte{0x50}, std::byte{0x4E}, std::byte{0x47},
        std::byte{0x0D}, std::byte{0x0A}, std::byte{0x1A}, std::byte{0x0A},
    };

    // The SOI marker, followed by the first byte of whichever marker comes next.
    constexpr std::array jpeg_signature{std::byte{0xFF}, std::byte{0xD8}, std::byte{0xFF}};

    if (std::ranges::starts_with(head, png_signature))
        return image_format::png;

    if (std::ranges::starts_with(head, jpeg_signature))
        return image_format::jpeg;

    return image_format::unknown;
}

}  // namespace arcana::data
