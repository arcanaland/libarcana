// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

// Which checks run for which codes.

#pragma once

#include "checks/ansi.hpp"
#include "checks/backs.hpp"
#include "checks/cards.hpp"
#include "checks/deck.hpp"
#include "checks/ids.hpp"
#include "checks/images.hpp"
#include "checks/names.hpp"
#include "checks/surrogate.hpp"
#include "context.hpp"

#include <arcana/deck.hpp>
#include <arcana/validation.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace arcana::validation
{

void no_check(check_context const& ctx);

// We are punting on the following for now:
// - `aspect-ratio-mismatch` needs an image decoder
// - `duplicate-deck-identifier` waiting for library phase
constexpr check_fn deferred = no_check;

// No check written yet.
constexpr check_fn pending = nullptr;

struct check_entry
{
    std::string_view code;
    check_fn run;
};

inline constexpr std::array checks{
    check_entry{.code = "ansi-outside-image-root", .run = check_ansi_outside_image_root},
    check_entry{.code = "aspect-ratio-mismatch", .run = deferred},
    check_entry{.code = "backslash-in-path", .run = pending},
    check_entry{.code = "bad-app-realm", .run = check_bad_app_realm},
    check_entry{.code = "bad-card-back-design-key", .run = check_bad_card_back_design_key},
    check_entry{.code = "bad-cards-table-key", .run = check_bad_cards_table_key},
    check_entry{.code = "bad-custom-name", .run = check_bad_custom_name},
    check_entry{.code = "bad-deck-identifier", .run = check_bad_deck_identifier},
    check_entry{.code = "bad-language-tag", .run = pending},
    check_entry{.code = "bad-link-rel", .run = pending},
    check_entry{.code = "bad-link-url", .run = pending},
    check_entry{.code = "bad-name-template-placeholder", .run = pending},
    check_entry{.code = "bad-palette-color", .run = pending},
    check_entry{.code = "bad-palette-snapped-color", .run = pending},
    check_entry{.code = "bad-rights-field-value", .run = pending},
    check_entry{.code = "bad-rights-status-uri", .run = pending},
    check_entry{.code = "bad-schema-version", .run = pending},
    check_entry{.code = "bad-signifies", .run = check_bad_signifies},
    check_entry{.code = "bad-spdx-expression", .run = pending},
    check_entry{.code = "bom-in-toml", .run = pending},
    check_entry{
        .code = "card-back-default-by-collation", .run = check_card_back_default_by_collation
    },
    check_entry{
        .code = "card-back-not-baseline-format", .run = check_card_back_not_baseline_format
    },
    check_entry{.code = "deck-has-no-cards", .run = pending},
    check_entry{.code = "deck-identifier-path-shape", .run = check_deck_identifier_path_shape},
    check_entry{.code = "declared-card-without-image", .run = pending},
    check_entry{.code = "deprecated-1-0-key", .run = pending},
    check_entry{.code = "duplicate-card-position", .run = pending},
    check_entry{.code = "duplicate-chain-extension", .run = check_duplicate_chain_extension},
    check_entry{.code = "duplicate-deck-identifier", .run = deferred},
    check_entry{.code = "duplicate-rank-in-ranks", .run = pending},
    check_entry{.code = "empty-card-number", .run = pending},
    check_entry{.code = "excluded-card-also-declared", .run = pending},
    check_entry{.code = "excluded-card-has-image", .run = pending},
    check_entry{.code = "ignored-card-back-file", .run = check_ignored_card_back_file},
    check_entry{.code = "ignored-image-root-lookalike", .run = check_ignored_image_root_lookalike},
    check_entry{.code = "language-tag-case-collision", .run = pending},
    check_entry{.code = "malformed-deck-toml", .run = pending},
    check_entry{.code = "malformed-name-file", .run = pending},
    check_entry{.code = "malformed-surrogate-file", .run = pending},
    check_entry{.code = "missing-alt-text", .run = pending},
    check_entry{.code = "missing-card-back-image", .run = check_missing_card_back_image},
    check_entry{.code = "missing-deck-identifier", .run = check_missing_deck_identifier},
    check_entry{.code = "missing-deck-toml", .run = pending},
    check_entry{.code = "missing-default-language-file", .run = pending},
    check_entry{.code = "missing-edition-default", .run = pending},
    check_entry{.code = "missing-license-file", .run = pending},
    check_entry{.code = "missing-license-text", .run = pending},
    check_entry{.code = "missing-packager", .run = pending},
    check_entry{.code = "missing-required-field", .run = pending},
    check_entry{.code = "missing-variant-image", .run = pending},
    check_entry{.code = "no-rights-statement", .run = pending},
    check_entry{.code = "non-canonical-card-reference", .run = check_non_canonical_card_reference},
    check_entry{.code = "non-canonical-language-tag", .run = pending},
    check_entry{.code = "non-utf8-name-file", .run = pending},
    check_entry{.code = "non-utf8-toml", .run = pending},
    check_entry{.code = "packager-equals-author", .run = pending},
    check_entry{.code = "palette-snapped-length-mismatch", .run = pending},
    check_entry{.code = "position-on-minor-arcanum", .run = pending},
    check_entry{.code = "rank-without-image", .run = pending},
    check_entry{.code = "raster-outside-image-root", .run = check_raster_outside_image_root},
    check_entry{.code = "redistribution-contradicts-rights-status", .run = pending},
    check_entry{.code = "redistribution-narrower-than-license", .run = pending},
    check_entry{.code = "reserved-custom-name", .run = check_reserved_custom_name},
    check_entry{.code = "signifies-self", .run = check_signifies_self},
    check_entry{.code = "stem-case-collision", .run = check_stem_case_collision},
    check_entry{.code = "surrogate-deck-redistribution-full", .run = pending},
    check_entry{.code = "surrogate-deck-without-buy-link", .run = pending},
    check_entry{.code = "surrogate-deck-without-license", .run = pending},
    check_entry{.code = "surrogate-deck-without-signifies", .run = pending},
    check_entry{.code = "svg-outside-scalable", .run = check_svg_outside_scalable},
    check_entry{.code = "symlink-escapes-deck-root", .run = pending},
    check_entry{.code = "unknown-default-card-back", .run = check_unknown_default_card_back},
    check_entry{.code = "unknown-edition-card-back", .run = check_unknown_edition_card_back},
    check_entry{.code = "unknown-edition-default", .run = pending},
    check_entry{.code = "unknown-metadata-alt-text-key", .run = pending},
    check_entry{.code = "unknown-name-key", .run = pending},
    check_entry{.code = "unknown-surrogate-key", .run = pending},
    check_entry{.code = "unknown-table", .run = pending},
    check_entry{.code = "unknown-variant-default", .run = pending},
    check_entry{.code = "unlocalized-fallback-string", .run = pending},
    check_entry{.code = "unnamed-extended-major", .run = pending},
    check_entry{.code = "unregistered-link-rel", .run = pending},
    check_entry{.code = "unsafe-path", .run = pending},
    check_entry{.code = "variant-card-without-default", .run = pending},
    check_entry{.code = "variant-for-unknown-card", .run = pending},
    check_entry{.code = "variant-missing-alt-text", .run = pending},
    check_entry{.code = "wrong-value-type", .run = pending},
};

// How many codes still have no check body.
[[nodiscard]] consteval std::size_t pending_checks()
{
    std::size_t count = 0;
    for (auto const& entry : checks)
        if (entry.run == pending)
            ++count;

    return count;
}

// How many codes are deliberately never implemented.
[[nodiscard]] consteval std::size_t deferred_checks()
{
    std::size_t count = 0;
    for (auto const& entry : checks)
        if (entry.run == deferred)
            ++count;

    return count;
}

static_assert(pending_checks() == 62, "work landed without the checks it required");

static_assert(deferred_checks() == 2, "only two punted checks");

static_assert(
    pending_checks() + deferred_checks() <= checks.size(), "the two sentinels must not overlap"
);

// Run every check whose rule applies, appending to `out` in catalogue order.
void run_all(
    deck const& d, std::uint8_t major, std::span<deck_file const> files,
    std::vector<diagnostic>& out
);

}  // namespace arcana::validation
