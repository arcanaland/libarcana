// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>
#include <optional>
#include <string_view>

namespace arcana::data
{

// The small frozen reference tables the deck checks read.
//
// Everything here is a lookup, never an inference. Where a table does not know
// an input it says so by returning nothing, and the caller stays silent: a
// wrong finding about someone's licensing is worse than no finding.

// --- CSS Color 4 named colours (DECK.md section 5.8.1) --------------------

// True when `name` is one of the 148 CSS Color 4 named colours.
//
// The set is frozen upstream. Comparison is bytewise: the spec writes the names
// in lower case and a surrogate file that shouts them is not conforming.
[[nodiscard]] bool is_css_color_name(std::string_view name) noexcept;

// --- The link relation registry (DECK.md section 4.1.1) -------------------

// True when `rel` is one of the five relations the specification registers.
[[nodiscard]] bool is_registered_link_rel(std::string_view rel) noexcept;

// True when `rel` uses the `x_` escape hatch section 4.1.1 documents for
// relations the registry does not define. Those are never reported: the
// registry is open and the prefix is what a deck author is told to use.
[[nodiscard]] bool is_extension_link_rel(std::string_view rel) noexcept;

// --- Rights status (DECK.md section 7.4) ----------------------------------

// What a rights-status URI says about the artwork it describes.
enum class rights_status_class : std::uint8_t
{
    // The artwork is in copyright, and the URI grants nothing.
    in_copyright,

    // The artwork is free of copyright, or dedicated to the public domain.
    no_copyright,

    // The URI is one that declines to say.
    undetermined,
};

// True when `uri` is a RightsStatements.org URI or a Creative Commons one,
// which is what section 7.4 says `rights_status` SHOULD be.
[[nodiscard]] bool is_rights_status_uri(std::string_view uri) noexcept;

// The classification of `uri`, or nothing when it is not a rights-status URI
// this table knows.
[[nodiscard]] std::optional<rights_status_class> classify_rights_status(
    std::string_view uri
) noexcept;

// --- ISO 639 shortest subtag (DECK.md section 6.1) ------------------------

// The two-letter ISO 639-1 code for a three-letter ISO 639-2 or -3 one, or
// nothing when the three-letter code has no shorter form.
//
// Section 6.1 says a tag SHOULD use the shortest available subtag: `en`, not
// `eng`. Both the bibliographic and the terminological three-letter codes map,
// so `ger` and `deu` both answer `de`.
[[nodiscard]] std::optional<std::string_view> shortest_language_subtag(
    std::string_view subtag
) noexcept;

// --- Curated licence permissions (DECK.md section 7.5) --------------------

// What a licence lets a downstream user do with the work it covers.
struct license_permissions
{
    bool grants_redistribution;
    bool grants_derivation;
};

// The permissions a licence grants, or nothing when it is outside the curated
// table.
//
// The SPDX License List carries no permissions matrix - it has isOsiApproved,
// isFsfLibre and isDeprecatedLicenseId and nothing about redistribution - and
// no canonical machine-readable source for one exists. So this is a small
// hand-curated allowlist covering the Creative Commons family, CC0 and the
// public-domain dedications, which is what a tarot deck actually carries.
//
// Returning nothing for everything else is the contract, not an omission.
// Growing the table is cheap; guessing is not.
[[nodiscard]] std::optional<license_permissions> find_license_permissions(
    std::string_view spdx_id
) noexcept;

}  // namespace arcana::data
