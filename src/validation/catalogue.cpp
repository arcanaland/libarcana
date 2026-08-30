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

[[nodiscard]] constexpr spec_section v1(std::string_view anchor) noexcept
{
    return {.schema_major = 1, .anchor = anchor};
}

[[nodiscard]] constexpr spec_section v2(std::string_view anchor) noexcept
{
    return {.schema_major = 2, .anchor = anchor};
}

// Sorted ascending by code
constexpr std::array catalogue{
    rule{
        .code = "ansi-outside-image-root",
        .default_level = severity::info,
        .area = "ansi",
        .needs = phase::filesystem,
        .cites = {v1("file-location-based-defaults"), v1("ansi-art"), v2("54-ansi-art")},
        .in_rules_table = false,
        .explanation = "This ANSI file is not under an ANSI image root and will be ignored.",
        .applies_to = {.min = 1, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "artwork-rating-exceeds-deck",
        .default_level = severity::error,
        .area = "deck",
        .needs = phase::document,
        .cites = {v2("416-content-rating")},
        .in_rules_table = true,
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
        .cites = {v2("41-deck")},
        .in_rules_table = true,
        .explanation =
            "This card asset's width-to-height ratio differs from the deck's declared aspect_ratio "
            "by more than a tenth.",
        .applies_to = {.min = 1, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "backslash-in-path",
        .default_level = severity::error,
        .area = "deck",
        .needs = phase::filesystem,
        .cites = {v2("23-file-format-and-encoding")},
        .in_rules_table = false,
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
        .cites = {v2("8-extensibility")},
        .in_rules_table = true,
        .explanation = "An app subtable key should be a quoted realm. Quote it.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "bad-card-back-design-key",
        .default_level = severity::error,
        .area = "backs",
        .needs = phase::document,
        .cites = {v2("55-card-back-images")},
        .in_rules_table = true,
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
        .cites = {v2("41-deck")},
        .in_rules_table = true,
        .explanation = "card_size_mm should be two numbers: width x height.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "bad-cards-table-key",
        .default_level = severity::error,
        .area = "ids",
        .needs = phase::document,
        .cites = {v2("312-card-references-and-the-variant-suffix"), v2("43-cards")},
        .in_rules_table = true,
        .explanation = "A key in the cards table is not a well-formed card reference.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "bad-container-entry-type",
        .default_level = severity::error,
        .area = "container",
        .needs = phase::library,
        .cites = {v2("24-deck-containers")},
        .in_rules_table = true,
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
        .cites = {v2("24-deck-containers")},
        .in_rules_table = true,
        .explanation = "A container does not have a deck.toml at its root.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "bad-content-rating-key",
        .default_level = severity::error,
        .area = "deck",
        .needs = phase::document,
        .cites = {v2("416-content-rating")},
        .in_rules_table = true,
        .explanation = "A content rating key is not a well-formed custom name.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "bad-custom-name",
        .default_level = severity::error,
        .area = "ids",
        .needs = phase::filesystem,
        .cites = {v2("32-custom-names"), v2("35-grammar")},
        .in_rules_table = true,
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
        .cites = {v2("33-qualified-identifiers"), v2("34-deck-identity")},
        .in_rules_table = true,
        .explanation = "The deck's identifier is not a well-formed qualified identifier: a realm, "
                       "a slash, and one or more path segments.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "bad-gtin",
        .default_level = severity::warning,
        .area = "deck",
        .needs = phase::document,
        .cites = {v2("415-product-identifiers")},
        .in_rules_table = true,
        .explanation = "GTIN needs to be eight, twelve, thirteen or fourteen digits.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "bad-isbn",
        .default_level = severity::warning,
        .area = "deck",
        .needs = phase::document,
        .cites = {v2("415-product-identifiers")},
        .in_rules_table = true,
        .explanation = "ISBN needs to be ten or thirteen characters.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "bad-language-tag",
        .default_level = severity::error,
        .area = "names",
        .needs = phase::filesystem,
        .cites = {v2("61-language-tags")},
        .in_rules_table = true,
        .explanation = "A name file's stem is not a well-formed BCP 47 language tag.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "bad-link-rel",
        .default_level = severity::error,
        .area = "deck",
        .needs = phase::document,
        .cites = {v2("411-links")},
        .in_rules_table = true,
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
        .cites = {v2("411-links")},
        .in_rules_table = true,
        .explanation = "A link's url is not an absolute http or https URL.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "bad-name-template-placeholder",
        .default_level = severity::error,
        .area = "names",
        .needs = phase::document,
        .cites = {v2("631-minor-arcana-name-composition")},
        .in_rules_table = true,
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
        .cites = {v2("416-content-rating")},
        .in_rules_table = true,
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
        .cites = {v2("581-the-surrogate-file")},
        .in_rules_table = true,
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
        .cites = {v2("581-the-surrogate-file")},
        .in_rules_table = true,
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
        .cites = {v2("414-pips")},
        .in_rules_table = true,
        .explanation = "The pips field is not one of scenic, emblematic or unstated.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "bad-product-id-key",
        .default_level = severity::error,
        .area = "deck",
        .needs = phase::document,
        .cites = {v2("415-product-identifiers")},
        .in_rules_table = true,
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
        .cites = {v2("35-grammar"), v2("417-published-date")},
        .in_rules_table = true,
        .explanation = "published_date is not a year, a year and month, or a full date denoting a "
                       "real calendar date.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "bad-related-entry",
        .default_level = severity::error,
        .area = "ids",
        .needs = phase::document,
        .cites = {v2("419-related-decks")},
        .in_rules_table = true,
        .explanation = "A [deck].related entry names a rel that is not a custom name, or a deck "
                       "that is not a well-formed qualified identifier or that carries a fragment. "
                       "A relation names a package, never a card.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "bad-rights-field-value",
        .default_level = severity::error,
        .area = "deck",
        .needs = phase::document,
        .cites = {v2("75-redistribution-and-derivation")},
        .in_rules_table = true,
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
        .cites = {v2("74-rights-status")},
        .in_rules_table = true,
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
        .cites = {v1("schema-versioning"), v2("14-versioning-and-compatibility")},
        .in_rules_table = true,
        .explanation = "The schema_version field is not two decimal integers separated by a dot. "
                       "Write it as a quoted string.",
        .applies_to = {.min = 1, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "bad-spdx-expression",
        .default_level = severity::warning,
        .area = "deck",
        .needs = phase::document,
        .cites = {v2("71-license-expressions")},
        .in_rules_table = true,
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
        .cites = {v2("23-file-format-and-encoding")},
        .in_rules_table = false,
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
        .cites = {v1("card-back-configuration"), v2("42-card_backs")},
        .in_rules_table = true,
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
        .cites = {v2("55-card-back-images"), v2("574-the-extension-chain")},
        .in_rules_table = true,
        .explanation =
            "A card back design is supplied in no baseline format, meaning none of PNG, "
            "JPEG or WebP. Backs have no reference deck to fall back on, so an application "
            "that cannot decode the design substitutes a generic back.",
        .applies_to = {.min = 1, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "card-not-baseline-format",
        .default_level = severity::warning,
        .area = "images",
        .needs = phase::filesystem,
        .cites = {v2("574-the-extension-chain")},
        .in_rules_table = true,
        .explanation = "There are no PNG, JPEG or WebP assets for a card.",
        .applies_to = {.min = 1, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "card-size-aspect-mismatch",
        .default_level = severity::warning,
        .area = "deck",
        .needs = phase::document,
        .cites = {v2("41-deck"), v2("56-aspect-ratio")},
        .in_rules_table = true,
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
        .cites = {v2("36-identifiers-in-toml"), v2("43-cards")},
        .in_rules_table = true,
        .explanation = "Entries in [cards] should be quoted card references.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "conflicting-deck-relation",
        .default_level = severity::error,
        .area = "ids",
        .needs = phase::document,
        .cites = {v2("419-related-decks"), v2("412-surrogate_for"), v2("413-follows")},
        .in_rules_table = true,
        .explanation = "A deck declares more than one follows relation or more than one "
                       "surrogate_for relation, or names the same deck under both. A deck that "
                       "stands in for another is that deck; it does not also resemble it.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "creator-equals-artist",
        .default_level = severity::warning,
        .area = "deck",
        .needs = phase::document,
        .cites = {v2("76-roles-and-credits")},
        .in_rules_table = true,
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
        .cites = {v2("91-conforming-deck")},
        .in_rules_table = false,
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
        .cites = {v2("33-qualified-identifiers")},
        .in_rules_table = true,
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
        .cites = {v2("416-content-rating")},
        .in_rules_table = true,
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
        .cites = {v2("43-cards")},
        .in_rules_table = true,
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
        .cites = {v2("appendix-b-reserved-and-deprecated-names"), v2("appendix-e-changelog")},
        .in_rules_table = false,
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
        .cites = {v2("432-ordering")},
        .in_rules_table = true,
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
        .cites = {v2("574-the-extension-chain")},
        .in_rules_table = true,
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
        .cites = {v2("223-shadowing"), v2("34-deck-identity")},
        .in_rules_table = true,
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
        .cites = {v2("44-suits")},
        .in_rules_table = true,
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
        .cites = {v2("431-card-numbers")},
        .in_rules_table = true,
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
        .cites = {v2("46-excluded_cards")},
        .in_rules_table = true,
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
        .cites = {v2("46-excluded_cards")},
        .in_rules_table = true,
        .explanation =
            "A card listed as excluded ships artwork anyway. Discovery reads the files rather than "
            "the declaration, so the card resolves. Remove it from the list, or remove its files.",
        .applies_to = {.min = 1, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "expands-with-excluded-cards",
        .default_level = severity::warning,
        .area = "deck",
        .needs = phase::filesystem,
        .cites = {v2("419-related-decks"), v2("576-when-no-asset-is-found")},
        .in_rules_table = true,
        .explanation =
            "A package declaring rel = \"expands\" also declares [excluded_cards] covering "
            "the canonical slots it does not carry. The relation already says the package is "
            "not a whole deck, so the exclusions restate it.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "ignored-card-back-file",
        .default_level = severity::warning,
        .area = "backs",
        .needs = phase::filesystem,
        .cites = {v2("55-card-back-images"), v2("572-extensions-stems-and-bases")},
        .in_rules_table = true,
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
        .cites = {v1("file-location-based-defaults"), v2("571-image-roots")},
        .in_rules_table = false,
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
        .cites = {v2("43-cards")},
        .in_rules_table = true,
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
        .cites = {v2("61-language-tags")},
        .in_rules_table = true,
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
        .cites = {v1("decktoml-schema"), v1("validation-rules"), v2("23-file-format-and-encoding")},
        .in_rules_table = true,
        .explanation = "The deck's deck.toml is not well-formed TOML 1.0.0.",
        .applies_to = {.min = 1, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "malformed-name-file",
        .default_level = severity::error,
        .area = "names",
        .needs = phase::filesystem,
        .cites =
            {v1("internationalization"), v1("validation-rules"), v2("23-file-format-and-encoding")},
        .in_rules_table = false,
        .explanation = "This name file is not well-formed TOML 1.0.0.",
        .applies_to = {.min = 1, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "malformed-surrogate-file",
        .default_level = severity::error,
        .area = "surrogate",
        .needs = phase::filesystem,
        .cites = {v2("581-the-surrogate-file")},
        .in_rules_table = true,
        .explanation = "This surrogate file is not well-formed TOML 1.0.0.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "missing-alt-text",
        .default_level = severity::warning,
        .area = "names",
        .needs = phase::filesystem,
        .cites = {v1("alt-text-guidelines"), v1("validation-rules"), v2("64-alt-text-guidelines")},
        .in_rules_table = true,
        .explanation =
            "No language file carries alt text for all of the deck's cards, so it is not usable "
            "with a screen reader.",
        .applies_to = {.min = 1, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "missing-artwork-complete",
        .default_level = severity::error,
        .area = "deck",
        .needs = phase::document,
        .cites = {v2("416-content-rating")},
        .in_rules_table = true,
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
        .cites = {v1("card-back-configuration"), v1("validation-rules"), v2("42-card_backs")},
        .in_rules_table = true,
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
        .cites = {v2("24-deck-containers")},
        .in_rules_table = true,
        .explanation = "A container's first entry is not an uncompressed mimetype file.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "missing-deck-identifier",
        .default_level = severity::warning,
        .area = "ids",
        .needs = phase::document,
        .cites = {v2("34-deck-identity")},
        .in_rules_table = true,
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
        .cites =
            {v1("directory-skeleton"), v1("validation-rules"), v2("23-file-format-and-encoding")},
        .in_rules_table = true,
        .explanation = "This directory contains no deck.toml and is not a deck at all.",
        .applies_to = {.min = 1, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "missing-default-language-file",
        .default_level = severity::error,
        .area = "names",
        .needs = phase::filesystem,
        .cites = {v2("61-language-tags")},
        .in_rules_table = true,
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
        .cites = {v2("72-attribution-and-notices")},
        .in_rules_table = true,
        .explanation = "A license_files entry names a file the deck does not ship.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "missing-license-text",
        .default_level = severity::warning,
        .area = "deck",
        .needs = phase::filesystem,
        .cites = {v2("72-attribution-and-notices")},
        .in_rules_table = false,
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
        .cites = {v2("76-roles-and-credits")},
        .in_rules_table = true,
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
        .cites = {v2("4-decktoml-reference")},
        .in_rules_table = true,
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
        .cites = {v2("43-cards")},
        .in_rules_table = true,
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
        .cites = {v2("7-licensing-and-attribution")},
        .in_rules_table = true,
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
        .cites = {v2("312-card-references-and-the-variant-suffix"), v2("46-excluded_cards")},
        .in_rules_table = false,
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
        .cites = {v2("61-language-tags")},
        .in_rules_table = false,
        .explanation = "This language tag is well-formed but not canonical. Rename the file.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "non-utf8-name-file",
        .default_level = severity::error,
        .area = "names",
        .needs = phase::filesystem,
        .cites = {v2("23-file-format-and-encoding")},
        .in_rules_table = false,
        .explanation = "This name file is not encoded as UTF-8.",
        .applies_to = {.min = 1, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "non-utf8-toml",
        .default_level = severity::error,
        .area = "deck",
        .needs = phase::filesystem,
        .cites = {v2("23-file-format-and-encoding")},
        .in_rules_table = false,
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
        .cites = {v2("76-roles-and-credits")},
        .in_rules_table = true,
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
        .cites = {v2("581-the-surrogate-file")},
        .in_rules_table = true,
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
        .cites = {v2("62-language-resolution"), v2("64-alt-text-guidelines")},
        .in_rules_table = true,
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
        .cites = {v2("43-cards")},
        .in_rules_table = true,
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
        .cites = {v2("44-suits")},
        .in_rules_table = true,
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
        .cites =
            {v1("file-location-based-defaults"), v1("raster-graphics"), v2("53-raster-graphics")},
        .in_rules_table = false,
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
        .cites = {v2("75-redistribution-and-derivation")},
        .in_rules_table = true,
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
        .cites = {v2("75-redistribution-and-derivation")},
        .in_rules_table = true,
        .explanation = "The deck's license grants redistribution or derivation outright and the "
                       "matching field claims less. The licence governs, so the field misleads a "
                       "reader without binding anyone.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "related-self",
        .default_level = severity::error,
        .area = "ids",
        .needs = phase::document,
        .cites = {v2("419-related-decks")},
        .in_rules_table = true,
        .explanation = "A [deck].related entry names this deck's own identifier. A deck stands in "
                       "no relation to itself, so the entry asserts nothing.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "reserved-custom-name",
        .default_level = severity::error,
        .area = "ids",
        .needs = phase::filesystem,
        .cites = {v2("32-custom-names"), v2("35-grammar")},
        .in_rules_table = true,
        .explanation =
            "A key the deck coined is one of the reserved canonical names: major_arcana, "
            "minor_arcana, the four canonical suits, or the fourteen canonical ranks.",
        .applies_to = {.min = 1, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "stem-case-collision",
        .default_level = severity::error,
        .area = "images",
        .needs = phase::filesystem,
        .cites = {v2("23-file-format-and-encoding")},
        .in_rules_table = true,
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
        .cites = {v2("59-surrogate-decks")},
        .in_rules_table = true,
        .explanation =
            "A surrogate deck declares redistribution full. The package carries no artwork to pass "
            "on, so it claims a permission over something it does not contain.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "surrogate-deck-unlinked",
        .default_level = severity::warning,
        .area = "surrogate",
        .needs = phase::filesystem,
        .cites = {v2("59-surrogate-decks"), v2("419-related-decks")},
        .in_rules_table = true,
        .explanation =
            "A surrogate deck declares neither a surrogate_for relation nor any [deck.product_ids] "
            "entry, so nothing connects it to the artwork it stands in for and its placeholders "
            "are shown even to a user who has the art.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "surrogate-deck-without-buy-link",
        .default_level = severity::warning,
        .area = "surrogate",
        .needs = phase::filesystem,
        .cites = {v2("59-surrogate-decks")},
        .in_rules_table = true,
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
        .cites = {v2("59-surrogate-decks")},
        .in_rules_table = true,
        .explanation =
            "A surrogate deck declares no license. The surrogates are the packager's own work and "
            "are the files the package carries, so a reader taking them up has no terms to go by.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "svg-outside-scalable",
        .default_level = severity::info,
        .area = "images",
        .needs = phase::filesystem,
        .cites =
            {v1("file-location-based-defaults"), v1("vector-graphics"), v2("52-vector-graphics")},
        .in_rules_table = false,
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
        .cites = {v2("101-path-traversal")},
        .in_rules_table = false,
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
        .cites = {v2("416-content-rating")},
        .in_rules_table = true,
        .explanation = "An artwork is rated under a system that the deck does not declare.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "unknown-default-card-back",
        .default_level = severity::error,
        .area = "backs",
        .needs = phase::filesystem,
        .cites = {v1("card-back-configuration"), v1("validation-rules"), v2("42-card_backs")},
        .in_rules_table = true,
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
        .cites = {v2("621-name-file-metadata")},
        .in_rules_table = true,
        .explanation = "A key in a name file's alt text metadata subtable is undefined.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "unknown-name-entity-kind",
        .default_level = severity::error,
        .area = "names",
        .needs = phase::filesystem,
        .cites = {v2("62-language-resolution")},
        .in_rules_table = true,
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
        .cites = {v2("62-language-resolution")},
        .in_rules_table = true,
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
        .cites = {v2("62-language-resolution"), v2("622-group-names")},
        .in_rules_table = true,
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
        .cites = {v2("581-the-surrogate-file")},
        .in_rules_table = true,
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
        .cites = {v2("4-decktoml-reference"), v2("8-extensibility")},
        .in_rules_table = false,
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
        .cites = {v2("43-cards")},
        .in_rules_table = true,
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
        .cites = {v2("42-card_backs"), v2("43-cards")},
        .in_rules_table = false,
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
        .cites = {v2("63-display-name-resolution")},
        .in_rules_table = true,
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
        .cites = {v2("416-content-rating")},
        .in_rules_table = true,
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
        .cites = {v2("411-links")},
        .in_rules_table = true,
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
        .cites = {v2("415-product-identifiers")},
        .in_rules_table = true,
        .explanation = "A product_ids scheme is outside the registry of isbn, gtin and "
                       "publisher_sku and is not prefixed with x_.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "unregistered-related-rel",
        .default_level = severity::warning,
        .area = "deck",
        .needs = phase::document,
        .cites = {v2("419-related-decks")},
        .in_rules_table = true,
        .explanation =
            "A [deck].related rel is outside the registry and is not prefixed. "
            "Applications ignore it, and a later version of this specification may claim "
            "the name. Prefix a relation of your own, as in x_kickstarter.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "unsafe-container-entry-name",
        .default_level = severity::error,
        .area = "container",
        .needs = phase::library,
        .cites = {v2("24-deck-containers")},
        .in_rules_table = true,
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
        .cites = {v2("23-file-format-and-encoding"), v2("101-path-traversal")},
        .in_rules_table = true,
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
        .cites = {v2("416-content-rating")},
        .in_rules_table = true,
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
        .cites = {v2("43-cards"), v2("575-variants")},
        .in_rules_table = true,
        .explanation = "A card has variant files, no unsuffixed file, and no declared default "
                       "variant, so a reference carrying no variant suffix names nothing.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "wrong-value-type",
        .default_level = severity::error,
        .area = "deck",
        .needs = phase::document,
        .cites = {v2("4-decktoml-reference")},
        .in_rules_table = true,
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

// What makes `pin_for`'s precondition hold: no rule cites a major spec_pin.hpp
// carries no text for.
static_assert(
    std::ranges::all_of(
        catalogue,
        [](rule const& r)
        {
            return std::ranges::all_of(
                r.citations(), [](spec_section const& s) { return has_pin(s.schema_major); }
            );
        }
    ),
    "a rule cites a schema major spec_pin.hpp does not pin"
);

static_assert(
    std::ranges::none_of(
        catalogue,
        [](rule const& r)
        {
            return std::ranges::any_of(
                r.cites,
                [](spec_section const& s)
                {
                    return s.schema_major == rule_table_section.schema_major &&
                           s.anchor == rule_table_section.anchor;
                }
            );
        }
    ),
    "cite the rule table by setting in_rules_table"
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
    if (!has_pin(schema_major))
        return {};

    return pin_for(schema_major).revision;
}

std::string url_of(spec_section section)
{
    if (!has_pin(section.schema_major))
        return {};

    spec_pin const& pin = pin_for(section.schema_major);

    constexpr std::string_view blob = "/blob/";

    std::string url;
    url.reserve(
        pin.repository.size() + blob.size() + pin.revision.size() + 1 + pin.file.size() + 1 +
        section.anchor.size()
    );

    url.append(pin.repository);
    url.append(blob);
    url.append(pin.revision);
    url.push_back('/');
    url.append(pin.file);
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
