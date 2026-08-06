// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>

namespace arcana::data
{

// Hand-written scanners for the grammars the deck specification defines.
//
// No regular-expression engine is involved, in the standard library or out of
// it. Every grammar below is a character-class scan over ASCII, which a
// hand-written scanner does in a few lines and reports better errors for.

// --- Identifiers (DECK.md sections 3.2, 3.3 and 3.5) ----------------------

// True when `s` matches the custom-name production: lowercase ASCII letters,
// digits and underscores, never starting with a digit.
[[nodiscard]] bool is_custom_name(std::string_view s) noexcept;

// True when `s` is one of the twenty reserved canonical keys section 3.2 says
// a custom name MUST NOT be: the two arcana, the four suits, the fourteen
// ranks. Grammar and reservation are separate questions and have separate
// diagnostics, so this does not imply is_custom_name.
[[nodiscard]] bool is_reserved_canonical_key(std::string_view s) noexcept;

// True when `s` is a canonical ID: major_arcana.<two digits or custom name>,
// or minor_arcana.<suit>.<rank>.
[[nodiscard]] bool is_canonical_id(std::string_view s) noexcept;

// True when `s` is a card reference: a canonical ID with an optional variant
// suffix, `:` followed by a variant key.
[[nodiscard]] bool is_card_reference(std::string_view s) noexcept;

// True when `s` is a variant reference, which is a card reference whose variant
// suffix is required rather than optional. Name files key [card_variants] by it.
[[nodiscard]] bool is_variant_reference(std::string_view s) noexcept;

// True when `s` is a realm: two or more dot-separated labels, each starting
// with a lowercase letter and neither starting nor ending with a hyphen.
[[nodiscard]] bool is_realm(std::string_view s) noexcept;

// The pieces of a qualified identifier.
//
// The views point into the string that was parsed, which must outlive this.
struct qualified_identifier
{
    std::string_view realm;
    std::string_view path;

    // Empty where the identifier carries none. A deck's fragment is a card
    // reference, but nothing here checks that: section 3.3 leaves the meaning
    // of a fragment to whichever specification owns the entity.
    std::string_view fragment;
};

// The pieces of `s`, or nothing when it is not a well-formed qualified
// identifier.
[[nodiscard]] std::optional<qualified_identifier> parse_qualified_identifier(
    std::string_view s
) noexcept;

// True when `s` is a well-formed qualified identifier.
[[nodiscard]] bool is_qualified_identifier(std::string_view s) noexcept;

// --- Language tags (DECK.md section 6.1) ----------------------------------

// True when `tag` is a well-formed BCP 47 language tag.
//
// Well-formed, not valid: section 6.1 asks for conformance to the RFC 5646
// grammar and nothing more, so no subtag is looked up in the IANA registry and
// no registry ships here. The grandfathered tags RFC 5646 lists are accepted
// because the grammar admits them; the irregular ones are matched by name.
[[nodiscard]] bool is_well_formed_language_tag(std::string_view tag);

// `tag` in the canonical form section 6.1 describes: the shortest available ISO
// 639 subtag, lowercase language, titlecase script, uppercase region, and lower
// case for everything else.
//
// Empty when `tag` is not well-formed. A grandfathered tag is returned as it
// was given: rewriting one to its preferred value needs the IANA registry.
[[nodiscard]] std::string canonicalize_language_tag(std::string_view tag);

// True when `tag` is well-formed and already in that canonical form.
[[nodiscard]] bool is_canonical_language_tag(std::string_view tag);

// --- Colours and URLs -----------------------------------------------------

// True when `s` is an sRGB hex triplet as section 5.8.1 writes them: a hash and
// exactly six lowercase hexadecimal digits. Upper case is not accepted.
[[nodiscard]] bool is_srgb_hex_triplet(std::string_view s) noexcept;

// True when `s` is an absolute URL with a scheme of http or https, which is
// what section 4.1.1 requires of a link's url.
//
// Well-formedness only. Nothing is fetched and no host is resolved.
[[nodiscard]] bool is_absolute_http_url(std::string_view s) noexcept;

// --- SPDX expressions (DECK.md section 7.1) -------------------------------

// The outcome of checking a `license` field.
//
// Section 7.1 states two obligations and this reports on both separately: the
// expression must be well-formed, and its identifiers must come from the SPDX
// License List.
struct spdx_expression_check
{
    // False when the expression does not parse. The identifier below is then
    // empty: an expression that does not parse has no identifiers to judge.
    bool well_formed;

    // The first identifier that is not on the list, or empty when every one is.
    // A view into the string that was checked.
    //
    // LicenseRef- and DocumentRef- identifiers are well-formed by construction
    // and are never looked up.
    std::string_view unknown_identifier;
};

// Check `s` against the SPDX license expression grammar and the vendored list.
[[nodiscard]] spdx_expression_check check_spdx_expression(std::string_view s);

// --- Image formats (DECK.md section 5.5) ----------------------------------

// The baseline raster formats a card back may be supplied in.
enum class image_format : std::uint8_t
{
    unknown,
    png,
    jpeg,
};

// The format `head` begins with, judged by its signature bytes.
//
// Signature bytes, not decoding: no image library enters this tree. `head` need
// only be the first few bytes of the file.
[[nodiscard]] image_format sniff_image_format(std::span<std::byte const> head) noexcept;

}  // namespace arcana::data
