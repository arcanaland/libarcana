// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

// The diagnostic catalogue.
//
// Derived from the Tarot Deck Specification v2

#include "catalogue.hpp"

#include "registry.hpp"
#include "spec_pin.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>

namespace arcana::validation
{

namespace
{

// The citations, one array per distinct list. Shared where two rules cite
// exactly the same sections; named for the first rule in code order that
// does. Every anchor is gated against the pinned text by spec_anchor_test.
constexpr std::array refs_ansi_outside_image_root{
    spec_section{.schema_major = 1, .anchor = "file-location-based-defaults"},
    spec_section{.schema_major = 1, .anchor = "ansi-art"},
    spec_section{.schema_major = 2, .anchor = "54-ansi-art"}
};
constexpr std::array refs_artwork_rating_exceeds_deck{
    spec_section{.schema_major = 2, .anchor = "416-content-rating"},
    spec_section{.schema_major = 2, .anchor = "94-validation-rules"}
};
constexpr std::array refs_aspect_ratio_mismatch{
    spec_section{.schema_major = 2, .anchor = "41-deck"},
    spec_section{.schema_major = 2, .anchor = "94-validation-rules"}
};
constexpr std::array refs_backslash_in_path{
    spec_section{.schema_major = 2, .anchor = "23-file-format-and-encoding"}
};
constexpr std::array refs_bad_app_realm{
    spec_section{.schema_major = 2, .anchor = "8-extensibility"},
    spec_section{.schema_major = 2, .anchor = "94-validation-rules"}
};
constexpr std::array refs_bad_card_back_design_key{
    spec_section{.schema_major = 2, .anchor = "55-card-back-images"},
    spec_section{.schema_major = 2, .anchor = "94-validation-rules"}
};
constexpr std::array refs_bad_cards_table_key{
    spec_section{.schema_major = 2, .anchor = "312-card-references-and-the-variant-suffix"},
    spec_section{.schema_major = 2, .anchor = "43-cards"},
    spec_section{.schema_major = 2, .anchor = "94-validation-rules"}
};
constexpr std::array refs_bad_container_entry_type{
    spec_section{.schema_major = 2, .anchor = "24-deck-containers"},
    spec_section{.schema_major = 2, .anchor = "94-validation-rules"}
};
constexpr std::array refs_bad_custom_name{
    spec_section{.schema_major = 2, .anchor = "32-custom-names"},
    spec_section{.schema_major = 2, .anchor = "35-grammar"},
    spec_section{.schema_major = 2, .anchor = "94-validation-rules"}
};
constexpr std::array refs_bad_deck_identifier{
    spec_section{.schema_major = 2, .anchor = "33-qualified-identifiers"},
    spec_section{.schema_major = 2, .anchor = "34-deck-identity"},
    spec_section{.schema_major = 2, .anchor = "94-validation-rules"}
};
constexpr std::array refs_bad_follows{
    spec_section{.schema_major = 2, .anchor = "413-follows"},
    spec_section{.schema_major = 2, .anchor = "94-validation-rules"}
};
constexpr std::array refs_bad_gtin{
    spec_section{.schema_major = 2, .anchor = "415-product-identifiers"},
    spec_section{.schema_major = 2, .anchor = "94-validation-rules"}
};
constexpr std::array refs_bad_language_tag{
    spec_section{.schema_major = 2, .anchor = "61-language-tags"},
    spec_section{.schema_major = 2, .anchor = "94-validation-rules"}
};
constexpr std::array refs_bad_link_rel{
    spec_section{.schema_major = 2, .anchor = "411-links"},
    spec_section{.schema_major = 2, .anchor = "94-validation-rules"}
};
constexpr std::array refs_bad_name_template_placeholder{
    spec_section{.schema_major = 2, .anchor = "631-minor-arcana-name-composition"},
    spec_section{.schema_major = 2, .anchor = "94-validation-rules"}
};
constexpr std::array refs_bad_palette_color{
    spec_section{.schema_major = 2, .anchor = "581-the-surrogate-file"},
    spec_section{.schema_major = 2, .anchor = "94-validation-rules"}
};
constexpr std::array refs_bad_pips_value{
    spec_section{.schema_major = 2, .anchor = "414-pips"},
    spec_section{.schema_major = 2, .anchor = "94-validation-rules"}
};
constexpr std::array refs_bad_published_date{
    spec_section{.schema_major = 2, .anchor = "35-grammar"},
    spec_section{.schema_major = 2, .anchor = "417-published-date"},
    spec_section{.schema_major = 2, .anchor = "94-validation-rules"}
};
constexpr std::array refs_bad_rights_field_value{
    spec_section{.schema_major = 2, .anchor = "75-redistribution-and-derivation"},
    spec_section{.schema_major = 2, .anchor = "94-validation-rules"}
};
constexpr std::array refs_bad_rights_status_uri{
    spec_section{.schema_major = 2, .anchor = "74-rights-status"},
    spec_section{.schema_major = 2, .anchor = "94-validation-rules"}
};
constexpr std::array refs_bad_schema_version{
    spec_section{.schema_major = 1, .anchor = "schema-versioning"},
    spec_section{.schema_major = 2, .anchor = "14-versioning-and-compatibility"},
    spec_section{.schema_major = 2, .anchor = "94-validation-rules"}
};
constexpr std::array refs_bad_signifies{
    spec_section{.schema_major = 2, .anchor = "412-signifies"},
    spec_section{.schema_major = 2, .anchor = "94-validation-rules"}
};
constexpr std::array refs_bad_spdx_expression{
    spec_section{.schema_major = 2, .anchor = "71-license-expressions"},
    spec_section{.schema_major = 2, .anchor = "94-validation-rules"}
};
constexpr std::array refs_card_back_default_by_collation{
    spec_section{.schema_major = 1, .anchor = "card-back-configuration"},
    spec_section{.schema_major = 2, .anchor = "42-card_backs"},
    spec_section{.schema_major = 2, .anchor = "94-validation-rules"}
};
constexpr std::array refs_card_back_not_baseline_format{
    spec_section{.schema_major = 2, .anchor = "55-card-back-images"},
    spec_section{.schema_major = 2, .anchor = "574-the-extension-chain"},
    spec_section{.schema_major = 2, .anchor = "94-validation-rules"}
};
constexpr std::array refs_card_not_baseline_format{
    spec_section{.schema_major = 2, .anchor = "574-the-extension-chain"},
    spec_section{.schema_major = 2, .anchor = "94-validation-rules"}
};
constexpr std::array refs_card_size_aspect_mismatch{
    spec_section{.schema_major = 2, .anchor = "41-deck"},
    spec_section{.schema_major = 2, .anchor = "56-aspect-ratio"},
    spec_section{.schema_major = 2, .anchor = "94-validation-rules"}
};
constexpr std::array refs_cards_key_path{
    spec_section{.schema_major = 2, .anchor = "36-identifiers-in-toml"},
    spec_section{.schema_major = 2, .anchor = "43-cards"},
    spec_section{.schema_major = 2, .anchor = "94-validation-rules"}
};
constexpr std::array refs_creator_equals_artist{
    spec_section{.schema_major = 2, .anchor = "76-roles-and-credits"},
    spec_section{.schema_major = 2, .anchor = "94-validation-rules"}
};
constexpr std::array refs_deck_has_no_cards{
    spec_section{.schema_major = 2, .anchor = "91-conforming-deck"}
};
constexpr std::array refs_deck_identifier_path_shape{
    spec_section{.schema_major = 2, .anchor = "33-qualified-identifiers"}
};
constexpr std::array refs_declared_card_without_image{
    spec_section{.schema_major = 2, .anchor = "43-cards"},
    spec_section{.schema_major = 2, .anchor = "94-validation-rules"}
};
constexpr std::array refs_deprecated_1_0_key{
    spec_section{.schema_major = 2, .anchor = "appendix-b-reserved-and-deprecated-names"},
    spec_section{.schema_major = 2, .anchor = "appendix-e-changelog"}
};
constexpr std::array refs_duplicate_card_position{
    spec_section{.schema_major = 2, .anchor = "432-ordering"},
    spec_section{.schema_major = 2, .anchor = "94-validation-rules"}
};
constexpr std::array refs_duplicate_deck_identifier{
    spec_section{.schema_major = 2, .anchor = "223-shadowing"},
    spec_section{.schema_major = 2, .anchor = "34-deck-identity"},
    spec_section{.schema_major = 2, .anchor = "94-validation-rules"}
};
constexpr std::array refs_duplicate_rank_in_ranks{
    spec_section{.schema_major = 2, .anchor = "44-suits"},
    spec_section{.schema_major = 2, .anchor = "94-validation-rules"}
};
constexpr std::array refs_empty_card_number{
    spec_section{.schema_major = 2, .anchor = "431-card-numbers"},
    spec_section{.schema_major = 2, .anchor = "94-validation-rules"}
};
constexpr std::array refs_excluded_card_also_declared{
    spec_section{.schema_major = 2, .anchor = "45-excluded_cards"},
    spec_section{.schema_major = 2, .anchor = "94-validation-rules"}
};
constexpr std::array refs_ignored_card_back_file{
    spec_section{.schema_major = 2, .anchor = "55-card-back-images"},
    spec_section{.schema_major = 2, .anchor = "572-extensions-stems-and-bases"},
    spec_section{.schema_major = 2, .anchor = "94-validation-rules"}
};
constexpr std::array refs_ignored_image_root_lookalike{
    spec_section{.schema_major = 1, .anchor = "file-location-based-defaults"},
    spec_section{.schema_major = 2, .anchor = "571-image-roots"}
};
constexpr std::array refs_malformed_deck_toml{
    spec_section{.schema_major = 1, .anchor = "decktoml-schema"},
    spec_section{.schema_major = 1, .anchor = "validation-rules"},
    spec_section{.schema_major = 2, .anchor = "23-file-format-and-encoding"},
    spec_section{.schema_major = 2, .anchor = "94-validation-rules"}
};
constexpr std::array refs_malformed_name_file{
    spec_section{.schema_major = 1, .anchor = "internationalization"},
    spec_section{.schema_major = 1, .anchor = "validation-rules"},
    spec_section{.schema_major = 2, .anchor = "23-file-format-and-encoding"}
};
constexpr std::array refs_missing_alt_text{
    spec_section{.schema_major = 1, .anchor = "alt-text-guidelines"},
    spec_section{.schema_major = 1, .anchor = "validation-rules"},
    spec_section{.schema_major = 2, .anchor = "64-alt-text-guidelines"},
    spec_section{.schema_major = 2, .anchor = "94-validation-rules"}
};
constexpr std::array refs_missing_card_back_image{
    spec_section{.schema_major = 1, .anchor = "card-back-configuration"},
    spec_section{.schema_major = 1, .anchor = "validation-rules"},
    spec_section{.schema_major = 2, .anchor = "42-card_backs"},
    spec_section{.schema_major = 2, .anchor = "94-validation-rules"}
};
constexpr std::array refs_missing_deck_identifier{
    spec_section{.schema_major = 2, .anchor = "34-deck-identity"},
    spec_section{.schema_major = 2, .anchor = "94-validation-rules"}
};
constexpr std::array refs_missing_deck_toml{
    spec_section{.schema_major = 1, .anchor = "directory-skeleton"},
    spec_section{.schema_major = 1, .anchor = "validation-rules"},
    spec_section{.schema_major = 2, .anchor = "23-file-format-and-encoding"},
    spec_section{.schema_major = 2, .anchor = "94-validation-rules"}
};
constexpr std::array refs_missing_license_file{
    spec_section{.schema_major = 2, .anchor = "72-attribution-and-notices"},
    spec_section{.schema_major = 2, .anchor = "94-validation-rules"}
};
constexpr std::array refs_missing_license_text{
    spec_section{.schema_major = 2, .anchor = "72-attribution-and-notices"}
};
constexpr std::array refs_missing_required_field{
    spec_section{.schema_major = 2, .anchor = "4-decktoml-reference"},
    spec_section{.schema_major = 2, .anchor = "94-validation-rules"}
};
constexpr std::array refs_no_rights_statement{
    spec_section{.schema_major = 2, .anchor = "7-licensing-and-attribution"},
    spec_section{.schema_major = 2, .anchor = "94-validation-rules"}
};
constexpr std::array refs_non_canonical_card_reference{
    spec_section{.schema_major = 2, .anchor = "312-card-references-and-the-variant-suffix"},
    spec_section{.schema_major = 2, .anchor = "45-excluded_cards"}
};
constexpr std::array refs_non_canonical_language_tag{
    spec_section{.schema_major = 2, .anchor = "61-language-tags"}
};
constexpr std::array refs_partial_alt_text_in_facet{
    spec_section{.schema_major = 2, .anchor = "62-language-resolution"},
    spec_section{.schema_major = 2, .anchor = "64-alt-text-guidelines"},
    spec_section{.schema_major = 2, .anchor = "94-validation-rules"}
};
constexpr std::array refs_raster_outside_image_root{
    spec_section{.schema_major = 1, .anchor = "file-location-based-defaults"},
    spec_section{.schema_major = 1, .anchor = "raster-graphics"},
    spec_section{.schema_major = 2, .anchor = "53-raster-graphics"}
};
constexpr std::array refs_stem_case_collision{
    spec_section{.schema_major = 2, .anchor = "23-file-format-and-encoding"},
    spec_section{.schema_major = 2, .anchor = "94-validation-rules"}
};
constexpr std::array refs_surrogate_deck_redistribution_full{
    spec_section{.schema_major = 2, .anchor = "59-surrogate-decks"},
    spec_section{.schema_major = 2, .anchor = "94-validation-rules"}
};
constexpr std::array refs_svg_outside_scalable{
    spec_section{.schema_major = 1, .anchor = "file-location-based-defaults"},
    spec_section{.schema_major = 1, .anchor = "vector-graphics"},
    spec_section{.schema_major = 2, .anchor = "52-vector-graphics"}
};
constexpr std::array refs_symlink_escapes_deck_root{
    spec_section{.schema_major = 2, .anchor = "101-path-traversal"}
};
constexpr std::array refs_unknown_metadata_alt_text_key{
    spec_section{.schema_major = 2, .anchor = "621-name-file-metadata"},
    spec_section{.schema_major = 2, .anchor = "94-validation-rules"}
};
constexpr std::array refs_unknown_name_entity_kind{
    spec_section{.schema_major = 2, .anchor = "62-language-resolution"},
    spec_section{.schema_major = 2, .anchor = "94-validation-rules"}
};
constexpr std::array refs_unknown_name_key{
    spec_section{.schema_major = 2, .anchor = "62-language-resolution"},
    spec_section{.schema_major = 2, .anchor = "622-group-names"},
    spec_section{.schema_major = 2, .anchor = "94-validation-rules"}
};
constexpr std::array refs_unknown_table{
    spec_section{.schema_major = 2, .anchor = "4-decktoml-reference"},
    spec_section{.schema_major = 2, .anchor = "8-extensibility"}
};
constexpr std::array refs_unlocalized_fallback_string{
    spec_section{.schema_major = 2, .anchor = "42-card_backs"},
    spec_section{.schema_major = 2, .anchor = "43-cards"}
};
constexpr std::array refs_unnamed_extended_major{
    spec_section{.schema_major = 2, .anchor = "63-display-name-resolution"},
    spec_section{.schema_major = 2, .anchor = "94-validation-rules"}
};
constexpr std::array refs_unsafe_path{
    spec_section{.schema_major = 2, .anchor = "23-file-format-and-encoding"},
    spec_section{.schema_major = 2, .anchor = "101-path-traversal"},
    spec_section{.schema_major = 2, .anchor = "94-validation-rules"}
};
constexpr std::array refs_variant_card_without_default{
    spec_section{.schema_major = 2, .anchor = "43-cards"},
    spec_section{.schema_major = 2, .anchor = "575-variants"},
    spec_section{.schema_major = 2, .anchor = "94-validation-rules"}
};

// Sorted ascending by code
constexpr std::array catalogue{
    rule{
        .code = "ansi-outside-image-root",
        .default_level = severity::info,
        .area = "ansi",
        .needs = phase::filesystem,
        .spec_refs = refs_ansi_outside_image_root,
        .explanation =
            "This ANSI file is not under an ANSI image root and is ignored. ANSI art is discovered "
            "only under a top-level directory named for the terminal rows it occupies.",
        .applies_to = {.min = 1, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "artwork-rating-exceeds-deck",
        .default_level = severity::error,
        .area = "deck",
        .needs = phase::document,
        .spec_refs = refs_artwork_rating_exceeds_deck,
        .explanation = "An artwork declares a content rating descriptor above the value the deck "
                       "declares. Either lower the artwork's rating or raise the deck's rating.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "aspect-ratio-mismatch",
        .default_level = severity::warning,
        .area = "images",
        .needs = phase::filesystem,
        .spec_refs = refs_aspect_ratio_mismatch,
        .explanation =
            "This card asset's width-to-height ratio differs from the deck's declared aspect_ratio "
            "by more than a tenth. Correct the artwork, or declare the ratio it actually has.",
        .applies_to = {.min = 1, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "backslash-in-path",
        .default_level = severity::error,
        .area = "deck",
        .needs = phase::filesystem,
        .spec_refs = refs_backslash_in_path,
        .explanation =
            "A path in deck.toml uses a backslash. Path-valued fields use forward slashes on every "
            "platform.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "bad-app-realm",
        .default_level = severity::error,
        .area = "ids",
        .needs = phase::document,
        .spec_refs = refs_bad_app_realm,
        .explanation =
            "An app subtable key is not a quoted realm. A realm contains a dot, so an unquoted key "
            "silently defines a subtable nested inside a subtable instead. Quote it.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "bad-card-back-design-key",
        .default_level = severity::error,
        .area = "backs",
        .needs = phase::document,
        .spec_refs = refs_bad_card_back_design_key,
        .explanation =
            "A key in the card back designs table is not a well-formed custom name: lowercase "
            "ASCII letters, digits and underscores, never starting with a digit.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "bad-card-size-mm",
        .default_level = severity::error,
        .area = "deck",
        .needs = phase::document,
        .spec_refs = refs_aspect_ratio_mismatch,
        .explanation = "card_size_mm should be two numbers: width x height.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "bad-cards-table-key",
        .default_level = severity::error,
        .area = "ids",
        .needs = phase::document,
        .spec_refs = refs_bad_cards_table_key,
        .explanation = "A key in the cards table is not a well-formed card reference.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "bad-container-entry-type",
        .default_level = severity::error,
        .area = "container",
        .needs = phase::library,
        .spec_refs = refs_bad_container_entry_type,
        .explanation = "A zip entry is a not a regular file or uses a "
                       "compression method other than stored or deflate.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "bad-container-layout",
        .default_level = severity::error,
        .area = "container",
        .needs = phase::library,
        .spec_refs = refs_bad_container_entry_type,
        .explanation = "A container does not have a deck.toml at its root.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "bad-content-rating-key",
        .default_level = severity::error,
        .area = "deck",
        .needs = phase::document,
        .spec_refs = refs_artwork_rating_exceeds_deck,
        .explanation = "A content rating key is not well-formed custom name.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "bad-custom-name",
        .default_level = severity::error,
        .area = "ids",
        .needs = phase::filesystem,
        .spec_refs = refs_bad_custom_name,
        .explanation = "A key custom key deck does not match the custom-name grammar: lowercase "
                       "ASCII letters, digits and underscores, never starting with a digit.",
        .applies_to = {.min = 1, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "bad-deck-identifier",
        .default_level = severity::error,
        .area = "ids",
        .needs = phase::document,
        .spec_refs = refs_bad_deck_identifier,
        .explanation = "The deck's identifier is not a well-formed qualified identifier: a realm, "
                       "a slash, and one or more path segments. It names the deck as a whole, so "
                       "it carries no fragment.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "bad-follows",
        .default_level = severity::error,
        .area = "ids",
        .needs = phase::document,
        .spec_refs = refs_bad_follows,
        .explanation = "The follows field is not a well-formed qualified identifier.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "bad-gtin",
        .default_level = severity::warning,
        .area = "deck",
        .needs = phase::document,
        .spec_refs = refs_bad_gtin,
        .explanation = "GTIN needs to be eight, twelve, thirteen or fourteen digits.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "bad-isbn",
        .default_level = severity::warning,
        .area = "deck",
        .needs = phase::document,
        .spec_refs = refs_bad_gtin,
        .explanation = "ISBN needs to be ten or thirteen characters.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "bad-language-tag",
        .default_level = severity::error,
        .area = "names",
        .needs = phase::filesystem,
        .spec_refs = refs_bad_language_tag,
        .explanation = "A name file's stem is not a well-formed BCP 47 language tag.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "bad-link-rel",
        .default_level = severity::error,
        .area = "deck",
        .needs = phase::document,
        .spec_refs = refs_bad_link_rel,
        .explanation = "A link's rel is not a well-formed custom name: lowercase ASCII letters, "
                       "digits and underscores, never starting with a digit.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "bad-link-url",
        .default_level = severity::error,
        .area = "deck",
        .needs = phase::document,
        .spec_refs = refs_bad_link_rel,
        .explanation = "A link's url is not an absolute http or https URL.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "bad-name-template-placeholder",
        .default_level = severity::error,
        .area = "names",
        .needs = phase::document,
        .spec_refs = refs_bad_name_template_placeholder,
        .explanation =
            "A minor arcana name template uses a placeholder other than the braced words rank and "
            "suit. Applications leave unknown braced text alone, so it reaches the user verbatim.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "bad-oars-descriptor",
        .default_level = severity::error,
        .area = "deck",
        .needs = phase::document,
        .spec_refs = refs_artwork_rating_exceeds_deck,
        .explanation = "An oras-1.1 descriptor is not one of the OARS 1.1 "
                       "attribute ids or its value is not one of <none, mild, moderate, intense>.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "bad-palette-color",
        .default_level = severity::error,
        .area = "surrogate",
        .needs = phase::filesystem,
        .spec_refs = refs_bad_palette_color,
        .explanation = "A surrogate palette entry is not an sRGB hex triplet: a hash followed by "
                       "exactly six lowercase hexadecimal digits. Upper case is not accepted.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "bad-palette-snapped-color",
        .default_level = severity::error,
        .area = "surrogate",
        .needs = phase::filesystem,
        .spec_refs = refs_bad_palette_color,
        .explanation = "A snapped palette entry is not a CSS Color 4 named colour. The field "
                       "exists so that an application with no colour arithmetic can render a "
                       "placeholder from the name alone.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "bad-pips-value",
        .default_level = severity::error,
        .area = "deck",
        .needs = phase::document,
        .spec_refs = refs_bad_pips_value,
        .explanation = "The pips field is not one of scenic, emblematic or unstated.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "bad-product-id-key",
        .default_level = severity::error,
        .area = "deck",
        .needs = phase::document,
        .spec_refs = refs_bad_gtin,
        .explanation = "A product_ids key is not a well-formed custom name, or its value is not a "
                       "non-empty string.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "bad-published-date",
        .default_level = severity::error,
        .area = "deck",
        .needs = phase::document,
        .spec_refs = refs_bad_published_date,
        .explanation = "published_date is not a year, a year and month, or a full date denoting a "
                       "real calendar date.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "bad-rights-field-value",
        .default_level = severity::error,
        .area = "deck",
        .needs = phase::document,
        .spec_refs = refs_bad_rights_field_value,
        .explanation =
            "The redistribution or derivation field carries a value other than full, surrogate, "
            "none or unstated. Omit the field to mean unstated, which grants nothing.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "bad-rights-status-uri",
        .default_level = severity::warning,
        .area = "deck",
        .needs = phase::document,
        .spec_refs = refs_bad_rights_status_uri,
        .explanation = "The rights_status value is not a RightsStatements.org or Creative Commons "
                       "URI.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "bad-schema-version",
        .default_level = severity::error,
        .area = "deck",
        .needs = phase::document,
        .spec_refs = refs_bad_schema_version,
        .explanation =
            "The schema_version field is not two decimal integers separated by a dot. The whole "
            "compatibility contract dispatches on this field. Write it as a quoted string.",
        .applies_to = {.min = 1, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "bad-signifies",
        .default_level = severity::error,
        .area = "ids",
        .needs = phase::document,
        .spec_refs = refs_bad_signifies,
        .explanation =
            "The signifies field is not a well-formed qualified identifier, or it carries a "
            "fragment. The value is a merge key against another package's identifier, which names "
            "a deck rather than a card.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "bad-spdx-expression",
        .default_level = severity::warning,
        .area = "deck",
        .needs = phase::document,
        .spec_refs = refs_bad_spdx_expression,
        .explanation = "The license field is not a well-formed SPDX expression. Terms with no SPDX "
                       "identifier are written as a custom LicenseRef- identifier, with the actual "
                       "terms recorded under license_files.",
        .applies_to = {.min = 1, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "bom-in-toml",
        .default_level = severity::warning,
        .area = "deck",
        .needs = phase::filesystem,
        .spec_refs = refs_backslash_in_path,
        .explanation = "This TOML file begins with a byte order mark, which is not part of TOML "
                       "1.0.0. A parser that does not skip it fails on the first key. Save the "
                       "file without a signature.",
        .applies_to = {.min = 1, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "card-back-default-by-collation",
        .default_level = severity::warning,
        .area = "backs",
        .needs = phase::filesystem,
        .spec_refs = refs_card_back_default_by_collation,
        .explanation = "The deck has several card back designs and declares no default, so the "
                       "default falls to the lexicographically first key. Declare a default, or "
                       "name the intended design default.",
        .applies_to = {.min = 1, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "card-back-not-baseline-format",
        .default_level = severity::warning,
        .area = "backs",
        .needs = phase::filesystem,
        .spec_refs = refs_card_back_not_baseline_format,
        .explanation = "A card back design is supplied in neither PNG nor JPEG. Backs have no "
                       "reference deck to fall back on, so an application that cannot decode the "
                       "design substitutes a generic back.",
        .applies_to = {.min = 1, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "card-not-baseline-format",
        .default_level = severity::warning,
        .area = "images",
        .needs = phase::filesystem,
        .spec_refs = refs_card_not_baseline_format,
        .explanation = "There are no PNG, JPEG or WebP assets for a card.",
        .applies_to = {.min = 1, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "card-size-aspect-mismatch",
        .default_level = severity::warning,
        .area = "deck",
        .needs = phase::document,
        .spec_refs = refs_card_size_aspect_mismatch,
        .explanation = "The width-to-height ratio of card_size_mm differs from the declared "
                       "aspect_ratio by more than a tenth.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "cards-key-path",
        .default_level = severity::error,
        .area = "ids",
        .needs = phase::document,
        .spec_refs = refs_cards_key_path,
        .explanation = "Entries in [cards] should be quoted card references.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "creator-equals-artist",
        .default_level = severity::warning,
        .area = "deck",
        .needs = phase::document,
        .spec_refs = refs_creator_equals_artist,
        .explanation = "The creator and artist fields are the same. Keep artist "
                       "and drop creator.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "deck-has-no-cards",
        .default_level = severity::error,
        .area = "deck",
        .needs = phase::filesystem,
        .spec_refs = refs_deck_has_no_cards,
        .explanation =
            "The deck has no card assets. Cards are discovered from the directory structure, so "
            "the artwork must sit under an image root, arranged by card type and suit.",
        .applies_to = {.min = 1, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "deck-identifier-path-shape",
        .default_level = severity::warning,
        .area = "ids",
        .needs = phase::document,
        .spec_refs = refs_deck_identifier_path_shape,
        .explanation = "The deck identifier's path is not the segment deck followed by the deck's "
                       "own name. The convention is what tells a deck's identifier from a "
                       "spread's. Another shape is not rejected.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "deck-rating-exceeds-artwork",
        .default_level = severity::warning,
        .area = "deck",
        .needs = phase::document,
        .spec_refs = refs_artwork_rating_exceeds_deck,
        .explanation =
            "The content rating of the deck is higher than the content rating of any card "
            " in the deck.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "declared-card-without-image",
        .default_level = severity::error,
        .area = "cards",
        .needs = phase::filesystem,
        .spec_refs = refs_declared_card_without_image,
        .explanation = "The cards table declares a card the deck ships no files for, so the "
                       "declaration annotates nothing. Canonical minor arcana and major arcana "
                       "keyed 00 through 21 are exempt.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "deprecated-1-0-key",
        .default_level = severity::info,
        .area = "deck",
        .needs = phase::document,
        .spec_refs = refs_deprecated_1_0_key,
        .explanation =
            "This key was defined by schema 1.0 and is not defined by 2.0, so applications ignore "
            "it and whatever it meant is silently lost. Appendix B names its 2.0 replacement.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "duplicate-card-position",
        .default_level = severity::warning,
        .area = "cards",
        .needs = phase::document,
        .spec_refs = refs_duplicate_card_position,
        .explanation =
            "Two major arcana declare the same position. Ordering stays well defined, since ties "
            "break by key, but the resulting order is not the one either declaration asked for.",
        .applies_to = {.min = 1, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "duplicate-chain-extension",
        .default_level = severity::warning,
        .area = "images",
        .needs = phase::filesystem,
        .spec_refs = refs_card_not_baseline_format,
        .explanation = "Two files in one directory share a stem and carry two different "
                       "extension-chain formats. Resolution is well defined, but one of the two is "
                       "usually a conversion tool's leftover.",
        .applies_to = {.min = 1, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "duplicate-deck-identifier",
        .default_level = severity::warning,
        .area = "ids",
        .needs = phase::library,
        .spec_refs = refs_duplicate_deck_identifier,
        .explanation = "Two visible decks in the library declare the same identifier. Both stay "
                       "visible and shadowing keys on directory name, so this is legitimate for "
                       "two installed versions or a fork.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "duplicate-rank-in-ranks",
        .default_level = severity::error,
        .area = "cards",
        .needs = phase::filesystem,
        .spec_refs = refs_duplicate_rank_in_ranks,
        .explanation =
            "A rank key appears twice in a suit's ranks list, which makes the order ambiguous. The "
            "list states ordering only, so the repeat adds no card. Remove the duplicate.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "empty-card-number",
        .default_level = severity::error,
        .area = "cards",
        .needs = phase::document,
        .spec_refs = refs_empty_card_number,
        .explanation =
            "A card's number field is an empty string. An unnumbered card follows from the shape "
            "of its key, not from an empty value. Write the printed number, or omit the field.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "excluded-card-also-declared",
        .default_level = severity::error,
        .area = "cards",
        .needs = phase::document,
        .spec_refs = refs_excluded_card_also_declared,
        .explanation =
            "A card is named both in the excluded cards table and in the custom cards table. The "
            "deck contradicts itself.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "excluded-card-has-image",
        .default_level = severity::warning,
        .area = "cards",
        .needs = phase::filesystem,
        .spec_refs = refs_excluded_card_also_declared,
        .explanation =
            "A card listed as excluded ships artwork anyway. Discovery reads the files rather than "
            "the declaration, so the card resolves. Remove it from the list, or remove its files.",
        .applies_to = {.min = 1, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "follows-self",
        .default_level = severity::error,
        .area = "ids",
        .needs = phase::document,
        .spec_refs = refs_bad_follows,
        .explanation =
            "The follows field cannot be this deck's own identifier or the deck it signifies.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "ignored-card-back-file",
        .default_level = severity::warning,
        .area = "backs",
        .needs = phase::filesystem,
        .spec_refs = refs_ignored_card_back_file,
        .explanation = "This file in a card back directory defines no design: its stem is not a "
                       "custom name, or its extension is outside the chain. Rename it, or point a "
                       "design's image path at it.",
        .applies_to = {.min = 1, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "ignored-image-root-lookalike",
        .default_level = severity::info,
        .area = "images",
        .needs = phase::filesystem,
        .spec_refs = refs_ignored_image_root_lookalike,
        .explanation = "This top-level directory nearly matches the image root pattern but is not "
                       "one, so discovery ignores it and its contents are not cards. It holds a "
                       "major or minor arcana subtree.",
        .applies_to = {.min = 1, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "ignored-key-on-variant",
        .default_level = severity::warning,
        .area = "cards",
        .needs = phase::document,
        .spec_refs = refs_declared_card_without_image,
        .explanation =
            "A number, position or default_variant is declared on a variant, which will be "
            "ignored.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "language-tag-case-collision",
        .default_level = severity::error,
        .area = "names",
        .needs = phase::filesystem,
        .spec_refs = refs_bad_language_tag,
        .explanation = "Two name files carry tags differing only in case. Applications compare "
                       "tags case-insensitively, so the deck resolves to one file on one platform "
                       "and the other elsewhere.",
        .applies_to = {.min = 1, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "malformed-deck-toml",
        .default_level = severity::error,
        .area = "deck",
        .needs = phase::filesystem,
        .spec_refs = refs_malformed_deck_toml,
        .explanation = "The deck's deck.toml is not well-formed TOML 1.0.0.",
        .applies_to = {.min = 1, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "malformed-name-file",
        .default_level = severity::error,
        .area = "names",
        .needs = phase::filesystem,
        .spec_refs = refs_malformed_name_file,
        .explanation = "This name file is not well-formed TOML 1.0.0.",
        .applies_to = {.min = 1, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "malformed-surrogate-file",
        .default_level = severity::error,
        .area = "surrogate",
        .needs = phase::filesystem,
        .spec_refs = refs_bad_palette_color,
        .explanation = "This surrogate file is not well-formed TOML 1.0.0.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "missing-alt-text",
        .default_level = severity::warning,
        .area = "names",
        .needs = phase::filesystem,
        .spec_refs = refs_missing_alt_text,
        .explanation =
            "No language file carries alt text for all of the deck's cards, so it is not usable "
            "with a screen reader. Write it under the default language file's alt text tables.",
        .applies_to = {.min = 1, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "missing-artwork-complete",
        .default_level = severity::error,
        .area = "deck",
        .needs = phase::document,
        .spec_refs = refs_artwork_rating_exceeds_deck,
        .explanation = "An artwork declares a content rating for a system that doesn't have "
                       "artwork_complete defined. It is ambiguous what is unrated and what is "
                       "unannotated.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "missing-card-back-image",
        .default_level = severity::error,
        .area = "backs",
        .needs = phase::filesystem,
        .spec_refs = refs_missing_card_back_image,
        .explanation =
            "A card back design's image path points at no file. The path overrides discovery for "
            "that design in every kind and size, so the design has no image at all.",
        .applies_to = {.min = 1, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "missing-container-mimetype",
        .default_level = severity::warning,
        .area = "container",
        .needs = phase::library,
        .spec_refs = refs_bad_container_entry_type,
        .explanation = "A container's first entry is not an uncompressed mimetype file.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "missing-deck-identifier",
        .default_level = severity::warning,
        .area = "ids",
        .needs = phase::document,
        .spec_refs = refs_missing_deck_identifier,
        .explanation = "The deck declares no identifier, so no other Arcana Land document can "
                       "reference it.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "missing-deck-toml",
        .default_level = severity::error,
        .area = "deck",
        .needs = phase::filesystem,
        .spec_refs = refs_missing_deck_toml,
        .explanation = "This directory contains no deck.toml and is not a deck at all.",
        .applies_to = {.min = 1, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "missing-default-language-file",
        .default_level = severity::error,
        .area = "names",
        .needs = phase::filesystem,
        .spec_refs = refs_bad_language_tag,
        .explanation = "The default_language field names a name file the deck does not ship. That "
                       "file is language resolution's last resort, so any string the requested tag "
                       "does not supply is lost.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "missing-license-file",
        .default_level = severity::error,
        .area = "deck",
        .needs = phase::filesystem,
        .spec_refs = refs_missing_license_file,
        .explanation = "A license_files entry names a file the deck does not ship.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "missing-license-text",
        .default_level = severity::warning,
        .area = "deck",
        .needs = phase::filesystem,
        .spec_refs = refs_missing_license_text,
        .explanation = "The deck declares a license but ships no license text. An SPDX expression "
                       "names terms without conveying them. List the text under license_files, or "
                       "put a LICENSE file at the deck root.",
        .applies_to = {.min = 1, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "missing-packager",
        .default_level = severity::warning,
        .area = "deck",
        .needs = phase::document,
        .spec_refs = refs_creator_equals_artist,
        .explanation = "The deck describes artwork it does not own and declares no packager, so "
                       "its assertions about that artwork are unattributable. Credit whoever "
                       "assembled this directory.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "missing-required-field",
        .default_level = severity::error,
        .area = "deck",
        .needs = phase::document,
        .spec_refs = refs_missing_required_field,
        .explanation =
            "A key whose Required column reads Yes is absent: schema_version, name and version in "
            "the deck table, rel and url on each links entry, or name on each edition.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "missing-variant-image",
        .default_level = severity::error,
        .area = "cards",
        .needs = phase::filesystem,
        .spec_refs = refs_declared_card_without_image,
        .explanation =
            "A variant's image path in the cards table points at no file, which leaves the variant "
            "unresolvable. Correct the path, or drop it and name the file by the convention.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "no-rights-statement",
        .default_level = severity::warning,
        .area = "deck",
        .needs = phase::document,
        .spec_refs = refs_no_rights_statement,
        .explanation = "The deck declares neither license nor rights_status. Declare a license "
                       "where there are terms to grant or a rights status where there are not.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "non-canonical-card-reference",
        .default_level = severity::error,
        .area = "ids",
        .needs = phase::document,
        .spec_refs = refs_non_canonical_card_reference,
        .explanation = "This card reference is not a canonical ID. Where a card is named rather "
                       "than a variant of one, a variant suffix is not accepted.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "non-canonical-language-tag",
        .default_level = severity::warning,
        .area = "names",
        .needs = phase::filesystem,
        .spec_refs = refs_non_canonical_language_tag,
        .explanation = "This language tag is well-formed but not canonical. Rename the file.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "non-utf8-name-file",
        .default_level = severity::error,
        .area = "names",
        .needs = phase::filesystem,
        .spec_refs = refs_backslash_in_path,
        .explanation = "This name file is not encoded as UTF-8.",
        .applies_to = {.min = 1, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "non-utf8-toml",
        .default_level = severity::error,
        .area = "deck",
        .needs = phase::filesystem,
        .spec_refs = refs_backslash_in_path,
        .explanation = "This deck.toml is not encoded as UTF-8. TOML 1.0.0 defines no other "
                       "encoding, so the file is not a TOML document whatever its bytes parse as.",
        .applies_to = {.min = 1, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "packager-equals-artist",
        .default_level = severity::warning,
        .area = "deck",
        .needs = phase::document,
        .spec_refs = refs_creator_equals_artist,
        .explanation = "The packager and artist fields are the same. The field exists to "
                       "distinguish whoever created the package from whoever made the artwork. "
                       "Drop packager or correct whichever is wrong.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "palette-snapped-length-mismatch",
        .default_level = severity::warning,
        .area = "surrogate",
        .needs = phase::filesystem,
        .spec_refs = refs_bad_palette_color,
        .explanation =
            "A surrogate's palette and snapped palette hold different numbers of entries, so an "
            "application pairing them by position pairs the wrong colours. Regenerate the file.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "partial-alt-text-in-facet",
        .default_level = severity::warning,
        .area = "names",
        .needs = phase::filesystem,
        .spec_refs = refs_partial_alt_text_in_facet,
        .explanation = "This name file gives alt text to some entities of a kind but not to this "
                       "one.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "position-on-minor-arcanum",
        .default_level = severity::warning,
        .area = "cards",
        .needs = phase::document,
        .spec_refs = refs_declared_card_without_image,
        .explanation =
            "A position is declared on a minor arcanum, where it is meaningless and ignored. A "
            "minor arcanum takes its place from its suit's ranks sequence instead.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "rank-without-image",
        .default_level = severity::error,
        .area = "cards",
        .needs = phase::filesystem,
        .spec_refs = refs_duplicate_rank_in_ranks,
        .explanation =
            "A rank named in a suit's ranks list has no artwork, so the list specifies a "
            "card the deck does not have. Add the artwork, or remove the rank from the list.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "raster-outside-image-root",
        .default_level = severity::info,
        .area = "images",
        .needs = phase::filesystem,
        .spec_refs = refs_raster_outside_image_root,
        .explanation = "This raster image is not under a height-named image root and is therefore "
                       "not a card asset. Discovery ignores it, and it will never be shown.",
        .applies_to = {.min = 1, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "redistribution-contradicts-rights-status",
        .default_level = severity::warning,
        .area = "deck",
        .needs = phase::document,
        .spec_refs = refs_bad_rights_field_value,
        .explanation =
            "The deck declares redistribution or derivation full while its rights_status says the "
            "artwork is in copyright with no licence granted. One of the two fields is wrong.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "redistribution-narrower-than-license",
        .default_level = severity::warning,
        .area = "deck",
        .needs = phase::document,
        .spec_refs = refs_bad_rights_field_value,
        .explanation = "The deck's license grants redistribution or derivation outright and the "
                       "matching field claims less. The licence governs, so the field misleads a "
                       "reader without binding anyone.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "reserved-custom-name",
        .default_level = severity::error,
        .area = "ids",
        .needs = phase::filesystem,
        .spec_refs = refs_bad_custom_name,
        .explanation =
            "A key the deck coined is one of the reserved canonical names: major_arcana, "
            "minor_arcana, the four canonical suits, or the fourteen canonical ranks.",
        .applies_to = {.min = 1, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "signifies-self",
        .default_level = severity::error,
        .area = "ids",
        .needs = phase::document,
        .spec_refs = refs_bad_signifies,
        .explanation =
            "The signifies field carries the deck's own identifier. The field names the package "
            "whose artwork this one describes, so pointing it here asserts nothing.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "stem-case-collision",
        .default_level = severity::error,
        .area = "images",
        .needs = phase::filesystem,
        .spec_refs = refs_stem_case_collision,
        .explanation = "Two files in one directory have stems differing only in case. Applications "
                       "compare stems case-insensitively, so this is one name with two files "
                       "behind it. Rename one of the two.",
        .applies_to = {.min = 1, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "surrogate-deck-redistribution-full",
        .default_level = severity::warning,
        .area = "surrogate",
        .needs = phase::filesystem,
        .spec_refs = refs_surrogate_deck_redistribution_full,
        .explanation =
            "A surrogate deck declares redistribution full. The package carries no artwork to pass "
            "on, so it claims a permission over something it does not contain.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "surrogate-deck-without-buy-link",
        .default_level = severity::warning,
        .area = "surrogate",
        .needs = phase::filesystem,
        .spec_refs = refs_surrogate_deck_redistribution_full,
        .explanation =
            "A surrogate deck declares neither a buy link nor a rights_status, so a reader sees "
            "placeholders without being told why, or where to obtain the artwork.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "surrogate-deck-without-license",
        .default_level = severity::warning,
        .area = "surrogate",
        .needs = phase::filesystem,
        .spec_refs = refs_surrogate_deck_redistribution_full,
        .explanation =
            "A surrogate deck declares no license. The surrogates are the packager's own work and "
            "are the files the package carries, so a reader taking them up has no terms to go by.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "surrogate-deck-without-signifies",
        .default_level = severity::warning,
        .area = "surrogate",
        .needs = phase::filesystem,
        .spec_refs = refs_surrogate_deck_redistribution_full,
        .explanation =
            "A surrogate deck declares no signifies, so nothing connects it to the artwork deck it "
            "stands in for and its placeholders are shown even to a user who has the art.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "svg-outside-scalable",
        .default_level = severity::info,
        .area = "images",
        .needs = phase::filesystem,
        .spec_refs = refs_svg_outside_scalable,
        .explanation = "This SVG is not under the scalable directory and is therefore not a card "
                       "asset. Discovery ignores it, and it will never be shown.",
        .applies_to = {.min = 1, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "symlink-escapes-deck-root",
        .default_level = severity::error,
        .area = "deck",
        .needs = phase::filesystem,
        .spec_refs = refs_symlink_escapes_deck_root,
        .explanation =
            "A symbolic link inside the deck leads outside the deck root. A deck "
            "arrives from outside the system, so this reads a file its packager chose on "
            "a machine they do not own.",
        .applies_to = {.min = 1, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "unknown-artwork-rating-system",
        .default_level = severity::error,
        .area = "deck",
        .needs = phase::document,
        .spec_refs = refs_artwork_rating_exceeds_deck,
        .explanation = "An artwork is rated under a system that the deck does not declare.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "unknown-default-card-back",
        .default_level = severity::error,
        .area = "backs",
        .needs = phase::filesystem,
        .spec_refs = refs_missing_card_back_image,
        .explanation = "The default declared under the card backs table names no design the deck "
                       "has. Designs are the stems found across the card back directories plus "
                       "every key with an explicit image path.",
        .applies_to = {.min = 1, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "unknown-metadata-alt-text-key",
        .default_level = severity::error,
        .area = "names",
        .needs = phase::document,
        .spec_refs = refs_unknown_metadata_alt_text_key,
        .explanation = "A key in a name file's alt text metadata subtable is undefined.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "unknown-name-entity-kind",
        .default_level = severity::error,
        .area = "names",
        .needs = phase::filesystem,
        .spec_refs = refs_unknown_name_entity_kind,
        .explanation = "Encountered an entity kind this specification does not define, which is "
                       "one of: card, suit, rank, card_back, variant and group.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "unknown-name-facet",
        .default_level = severity::error,
        .area = "names",
        .needs = phase::filesystem,
        .spec_refs = refs_unknown_name_entity_kind,
        .explanation =
            "A top-level table in a name file must be one of name, alt_text or metadata.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "unknown-name-key",
        .default_level = severity::error,
        .area = "names",
        .needs = phase::filesystem,
        .spec_refs = refs_unknown_name_key,
        .explanation = "A key in this name file corresponds to nothing the deck has, so the string "
                       "it carries will never be shown. The card was probably renamed, misspelled, "
                       "or copied from another deck.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "unknown-surrogate-key",
        .default_level = severity::error,
        .area = "surrogate",
        .needs = phase::filesystem,
        .spec_refs = refs_bad_palette_color,
        .explanation = "A surrogate file carries a key other than palette, palette_snapped and "
                       "thumbhash. Surrogate files are generated, so an unexpected key means the "
                       "generator and the reader disagree.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "unknown-table",
        .default_level = severity::info,
        .area = "deck",
        .needs = phase::document,
        .spec_refs = refs_unknown_table,
        .explanation =
            "This top-level table is not one the specification defines and is not the app table, "
            "so it is reserved for a future version and ignored.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "unknown-variant-default",
        .default_level = severity::error,
        .area = "cards",
        .needs = phase::filesystem,
        .spec_refs = refs_declared_card_without_image,
        .explanation = "A card's declared default variant names no variant that card has, which "
                       "leaves a reference carrying no variant suffix unresolvable.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "unlocalized-fallback-string",
        .default_level = severity::warning,
        .area = "names",
        .needs = phase::document,
        .spec_refs = refs_unlocalized_fallback_string,
        .explanation =
            "A name or alt text  only exists in deck.toml, where it cannot be translated."
            " Write it in the deck's name files as well.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "unnamed-extended-major",
        .default_level = severity::warning,
        .area = "names",
        .needs = phase::filesystem,
        .spec_refs = refs_unnamed_extended_major,
        .explanation = "A major arcanum keyed above 21 is named in no name file, so it is shown to "
                       "the user as a bare number. Nothing else can name it.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "unregistered-content-rating-system",
        .default_level = severity::warning,
        .area = "deck",
        .needs = phase::document,
        .spec_refs = refs_artwork_rating_exceeds_deck,
        .explanation =
            "A content rating system is outside the registry and is not prefixed with x_.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "unregistered-link-rel",
        .default_level = severity::warning,
        .area = "deck",
        .needs = phase::document,
        .spec_refs = refs_bad_link_rel,
        .explanation = "This link relation is not in the registry. The registry is open and "
                       "applications ignore what they do not recognise, but a later version may "
                       "claim the name. Prefix your own, as in x_kickstarter.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "unregistered-product-id-scheme",
        .default_level = severity::warning,
        .area = "deck",
        .needs = phase::document,
        .spec_refs = refs_bad_gtin,
        .explanation = "A product_ids scheme is outside the registry of isbn, gtin and "
                       "publisher_sku and is not prefixed with x_.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "unsafe-container-entry-name",
        .default_level = severity::error,
        .area = "container",
        .needs = phase::library,
        .spec_refs = refs_bad_container_entry_type,
        .explanation = "A container entry name is absolute, names a drive, carries a dot-dot, dot "
                       "or empty segment, or repeats another entry.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "unsafe-path",
        .default_level = severity::error,
        .area = "deck",
        .needs = phase::filesystem,
        .spec_refs = refs_unsafe_path,
        .explanation = "A path in deck.toml begins with a slash, contains a parent-directory "
                       "segment, or resolves outside the deck root. Applications must reject such "
                       "a path rather than resolve it.",
        .applies_to = {.min = 1, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "unused-artwork-complete",
        .default_level = severity::warning,
        .area = "deck",
        .needs = phase::document,
        .spec_refs = refs_artwork_rating_exceeds_deck,
        .explanation = "An artwork_complete is declared on a system that no artwork has a "
                       "descriptor for.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "variant-card-without-default",
        .default_level = severity::error,
        .area = "cards",
        .needs = phase::filesystem,
        .spec_refs = refs_variant_card_without_default,
        .explanation = "A card has variant files, no unsuffixed file, and no declared default "
                       "variant, so a reference carrying no variant suffix names nothing.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "variant-for-unknown-card",
        .default_level = severity::error,
        .area = "cards",
        .needs = phase::filesystem,
        .spec_refs = refs_declared_card_without_image,
        .explanation = "A card variants table is keyed on a card the deck does not have, so it "
                       "annotates nothing. A card exists because the deck ships an asset for it.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "wrong-value-type",
        .default_level = severity::error,
        .area = "deck",
        .needs = phase::document,
        .spec_refs = refs_missing_required_field,
        .explanation = "A key carries a value of a type other than the one its field table gives. "
                       "The usual case is a date: created_date and updated_date are strings, so an "
                       "unquoted date is wrong. Quote it.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
};

// needed for std::lower_bound
static_assert(
    std::ranges::is_sorted(catalogue, {}, &rule::code),
    "the catalogue must be sorted ascending by code"
);

static_assert(
    std::ranges::adjacent_find(catalogue, {}, &rule::code) == catalogue.end(),
    "the catalogue shouldn't have duplicate codes"
);

[[nodiscard]] consteval std::size_t rules_at_major_one()
{
    return std::count_if(
        std::begin(catalogue), std::end(catalogue),
        [](auto const& rule) { return rule.applies_to.min == 1; }
    );
}

constexpr std::size_t known_v1_rules = 31;
static_assert(
    rules_at_major_one() == known_v1_rules,
    "a rule changed major without the criterion being re-read"
);

consteval bool checks_cover_catalogue()
{
    if (checks.size() != catalogue.size())
        return false;

    for (std::size_t i = 0; i < catalogue.size(); ++i)
        if (checks[i].code != catalogue[i].code)
            return false;

    return true;
}

static_assert(
    checks_cover_catalogue(), "every rule needs a row in the dispatch table in catalogue order"
);

[[nodiscard]] std::optional<std::size_t> index_of(std::string_view code) noexcept
{
    auto const* const found = std::ranges::lower_bound(catalogue, code, {}, &rule::code);
    if (found == catalogue.end() || found->code != code)
        return std::nullopt;

    return static_cast<std::size_t>(found - catalogue.begin());
}

}  // namespace

std::string_view revision_of(std::uint8_t schema_major) noexcept
{
    spec_pin const* const pin = pin_for(schema_major);

    return pin == nullptr ? std::string_view{} : pin->revision;
}

std::string url_of(spec_section section)
{
    spec_pin const* const pin = pin_for(section.schema_major);
    if (pin == nullptr)
        return {};

    constexpr std::string_view blob = "/blob/";

    std::string url;
    url.reserve(
        pin->repository.size() + blob.size() + pin->revision.size() + 1 + pin->file.size() + 1 +
        section.anchor.size()
    );

    url.append(pin->repository);
    url.append(blob);
    url.append(pin->revision);
    url.push_back('/');
    url.append(pin->file);
    url.push_back('#');
    url.append(section.anchor);

    return url;
}

std::span<rule const> all_rules() noexcept
{
    return catalogue;
}

rule const* lookup(std::string_view code) noexcept
{
    std::optional<std::size_t> const found = index_of(code);
    if (!found.has_value())
        return nullptr;

    return &catalogue[*found];
}

std::optional<rule_state> state_of_code(std::string_view code) noexcept
{
    std::optional<std::size_t> const found = index_of(code);
    if (!found)
        return std::nullopt;

    check_fn const run = checks[*found].run;

    if (run == pending)
        return rule_state::pending;

    if (run == deferred)
        return rule_state::deferred;

    return rule_state::checked;
}

}  // namespace arcana::validation
