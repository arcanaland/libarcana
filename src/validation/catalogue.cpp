// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

// The diagnostic catalogue.
//
// Derived from the Tarot Deck Specification v2
//
// This is the one translation unit that sees both the catalogue and the
// dispatch table, so it is where the two static_asserts tying them together
// live.

#include "catalogue.hpp"

#include "registry.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <span>
#include <string_view>

namespace arcana::validation
{

namespace
{

// One entry per distinct code, sorted ascending by code.
constexpr std::array catalogue{
    rule{
        .code = "ansi-outside-image-root",
        .default_level = severity::info,
        .area = "ansi",
        .needs = phase::filesystem,
        .spec_ref = "DECK.md#5.4",
        .explanation =
            "This ANSI file is not under an ANSI image root and is ignored. ANSI art is discovered "
            "only under a top-level directory named for the terminal rows it occupies.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "artwork-rating-exceeds-deck",
        .default_level = severity::error,
        .area = "deck",
        .needs = phase::document,
        .spec_ref = "DECK.md#4.1.6; DECK.md#9.4",
        .explanation = "An artwork declares a content rating descriptor above the value the deck "
                       "declares for the "
                       "same system and descriptor. The deck level states the ceiling, so raise it "
                       "or lower the "
                       "artwork.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "aspect-ratio-mismatch",
        .default_level = severity::warning,
        .area = "images",
        .needs = phase::filesystem,
        .spec_ref = "DECK.md#4.1; DECK.md#9.4",
        .explanation =
            "This card asset's width-to-height ratio differs from the deck's declared aspect_ratio "
            "by more than a tenth. Correct the artwork, or declare the ratio it actually has.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "backslash-in-path",
        .default_level = severity::error,
        .area = "deck",
        .needs = phase::filesystem,
        .spec_ref = "DECK.md#2.3",
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
        .spec_ref = "DECK.md#8; DECK.md#9.4",
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
        .spec_ref = "DECK.md#5.5; DECK.md#9.4",
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
        .spec_ref = "DECK.md#4.1; DECK.md#9.4",
        .explanation = "card_size_mm does not hold exactly two numbers greater than zero. It is "
                       "the physical width "
                       "then the height in millimetres.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "bad-cards-table-key",
        .default_level = severity::error,
        .area = "ids",
        .needs = phase::document,
        .spec_ref = "DECK.md#3.1.2; DECK.md#4.3; DECK.md#9.4",
        .explanation = "A key in the cards table is not a well-formed card reference. A major "
                       "arcanum's key carries "
                       "both of its digits, and a variant reference's suffix is a custom name.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "bad-container-entry-type",
        .default_level = severity::error,
        .area = "container",
        .needs = phase::library,
        .spec_ref = "DECK.md#2.4; DECK.md#9.4",
        .explanation = "A container entry is a symbolic link, a hard link, an encrypted entry, or "
                       "something other "
                       "than a regular file or directory, or it uses a compression method other "
                       "than stored or "
                       "deflate.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "bad-container-layout",
        .default_level = severity::error,
        .area = "container",
        .needs = phase::library,
        .spec_ref = "DECK.md#2.4; DECK.md#9.4",
        .explanation = "A container does not carry deck.toml at the root of the archive. A deck "
                       "sitting inside a "
                       "wrapping directory is not a container.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "bad-content-rating-key",
        .default_level = severity::error,
        .area = "deck",
        .needs = phase::document,
        .spec_ref = "DECK.md#4.1.6; DECK.md#9.4",
        .explanation =
            "A content rating system key or descriptor key is not a well-formed custom name, or a "
            "descriptor carries no non-empty string, or artwork_complete is not a boolean or "
            "appears "
            "somewhere other than the deck level.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "bad-custom-name",
        .default_level = severity::error,
        .area = "ids",
        .needs = phase::filesystem,
        .spec_ref = "DECK.md#3.2; DECK.md#3.5; DECK.md#9.4",
        .explanation =
            "A key created by the deck author does not match the custom-name grammar: lowercase "
            "ASCII letters, digits and underscores, never starting with a digit. Rename it.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "bad-deck-identifier",
        .default_level = severity::error,
        .area = "ids",
        .needs = phase::document,
        .spec_ref = "DECK.md#3.3; DECK.md#3.4; DECK.md#9.4",
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
        .spec_ref = "DECK.md#4.1.3; DECK.md#9.4",
        .explanation =
            "The follows field is not a well-formed qualified identifier, or it carries a "
            "fragment. It "
            "names the deck this one is patterned on, as a whole, so it carries no fragment.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "bad-gtin",
        .default_level = severity::warning,
        .area = "deck",
        .needs = phase::document,
        .spec_ref = "DECK.md#4.1.5; DECK.md#9.4",
        .explanation = "A gtin is not eight, twelve, thirteen or fourteen digits, contains a "
                       "character other than a "
                       "digit, or fails its check digit. Box copy is transcribed by hand and this "
                       "is the error that "
                       "transcription makes.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "bad-isbn",
        .default_level = severity::warning,
        .area = "deck",
        .needs = phase::document,
        .spec_ref = "DECK.md#4.1.5; DECK.md#9.4",
        .explanation = "An isbn is not ten or thirteen characters once hyphens and spaces are "
                       "removed, or fails its "
                       "check digit. Box copy is transcribed by hand and this is the error that "
                       "transcription makes.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "bad-language-tag",
        .default_level = severity::error,
        .area = "names",
        .needs = phase::filesystem,
        .spec_ref = "DECK.md#6.1; DECK.md#9.4",
        .explanation =
            "A name file's stem is not a well-formed BCP 47 language tag, so language resolution "
            "never selects it. Rename the file to the tag itself, such as en.toml or pt-BR.toml.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "bad-link-rel",
        .default_level = severity::error,
        .area = "deck",
        .needs = phase::document,
        .spec_ref = "DECK.md#4.1.1; DECK.md#9.4",
        .explanation = "A link's rel is not a well-formed custom name: lowercase ASCII letters, "
                       "digits and underscores, never starting with a digit..",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "bad-link-url",
        .default_level = severity::error,
        .area = "deck",
        .needs = phase::document,
        .spec_ref = "DECK.md#4.1.1; DECK.md#9.4",
        .explanation = "A link's url is not an absolute http or https URL. A deck is a directory "
                       "on disk, so a relative reference has nothing to resolve against.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "bad-name-template-placeholder",
        .default_level = severity::error,
        .area = "names",
        .needs = phase::document,
        .spec_ref = "DECK.md#6.3.1; DECK.md#9.4",
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
        .spec_ref = "DECK.md#4.1.6; DECK.md#9.4",
        .explanation =
            "A descriptor under an oars-1.1 subtable is not one of the twenty-two OARS 1.1 "
            "attribute ids "
            "written with underscores, or its value is not one of none, mild, moderate or intense.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "bad-palette-color",
        .default_level = severity::error,
        .area = "surrogate",
        .needs = phase::filesystem,
        .spec_ref = "DECK.md#5.8.1; DECK.md#9.4",
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
        .spec_ref = "DECK.md#5.8.1; DECK.md#9.4",
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
        .spec_ref = "DECK.md#4.1.4; DECK.md#9.4",
        .explanation = "The pips field is not one of scenic, emblematic or unstated. A deck whose "
                       "suits differ among "
                       "themselves declares nothing rather than coining a fourth value.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "bad-product-id-key",
        .default_level = severity::error,
        .area = "deck",
        .needs = phase::document,
        .spec_ref = "DECK.md#4.1.5; DECK.md#9.4",
        .explanation = "A product_ids key is not a well-formed custom name, or its value is not a "
                       "non-empty string. "
                       "The key names the identifier scheme and the value is the identifier.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "bad-published-date",
        .default_level = severity::error,
        .area = "deck",
        .needs = phase::document,
        .spec_ref = "DECK.md#3.5; DECK.md#4.1.7; DECK.md#9.4",
        .explanation = "published_date is not a year, a year and month, or a full date denoting a "
                       "real calendar "
                       "date. State no precision you do not have: the year alone is the correct "
                       "value where the day "
                       "is unknown.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "bad-rights-field-value",
        .default_level = severity::error,
        .area = "deck",
        .needs = phase::document,
        .spec_ref = "DECK.md#7.5; DECK.md#9.4",
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
        .spec_ref = "DECK.md#7.4; DECK.md#9.4",
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
        .spec_ref = "DECK.md#1.4; DECK.md#9.4",
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
        .spec_ref = "DECK.md#4.1.2; DECK.md#9.4",
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
        .spec_ref = "DECK.md#7.1; DECK.md#9.4",
        .explanation = "The license field is not a well-formed SPDX expression. Terms with no SPDX "
                       "identifier are written as a custom LicenseRef- identifier, with the actual "
                       "terms recorded under license_files.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "bom-in-toml",
        .default_level = severity::warning,
        .area = "deck",
        .needs = phase::filesystem,
        .spec_ref = "DECK.md#2.3",
        .explanation = "This TOML file begins with a byte order mark, which is not part of TOML "
                       "1.0.0. A parser that does not skip it fails on the first key. Save the "
                       "file without a signature.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "card-back-default-by-collation",
        .default_level = severity::warning,
        .area = "backs",
        .needs = phase::filesystem,
        .spec_ref = "DECK.md#4.2; DECK.md#9.4",
        .explanation = "The deck has several card back designs and declares no default, so the "
                       "default falls to the lexicographically first key. Declare a default, or "
                       "name the intended design default.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "card-back-not-baseline-format",
        .default_level = severity::warning,
        .area = "backs",
        .needs = phase::filesystem,
        .spec_ref = "DECK.md#5.5; DECK.md#5.7.4; DECK.md#9.4",
        .explanation = "A card back design is supplied in neither PNG nor JPEG. Backs have no "
                       "reference deck to fall back on, so an application that cannot decode the "
                       "design substitutes a generic back.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "card-not-baseline-format",
        .default_level = severity::warning,
        .area = "images",
        .needs = phase::filesystem,
        .spec_ref = "DECK.md#5.7.4; DECK.md#9.4",
        .explanation =
            "Every raster asset this card has, across all image roots, is in a format outside the "
            "baseline of PNG, JPEG and WebP, so an application that decodes only the baseline "
            "falls back "
            "to a reference deck or shows nothing.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "card-size-aspect-mismatch",
        .default_level = severity::warning,
        .area = "deck",
        .needs = phase::document,
        .spec_ref = "DECK.md#4.1; DECK.md#5.6; DECK.md#9.4",
        .explanation = "The width-to-height ratio of card_size_mm differs from the declared "
                       "aspect_ratio by more "
                       "than a tenth. One of the two is likely a transcription error, though "
                       "aspect_ratio governs "
                       "either way.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "cards-key-path",
        .default_level = severity::error,
        .area = "ids",
        .needs = phase::document,
        .spec_ref = "DECK.md#3.6; DECK.md#4.3; DECK.md#9.4",
        .explanation = "A cards entry is written as a key path rather than a single key, so it "
                       "declares a table "
                       "named for the card's first segment rather than the card. Quote the whole "
                       "card reference.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "deck-has-no-cards",
        .default_level = severity::error,
        .area = "deck",
        .needs = phase::filesystem,
        .spec_ref = "DECK.md#9.1",
        .explanation =
            "The deck has no card assets. Cards are discovered from the directory structure, so "
            "the artwork must sit under an image root, arranged by card type and suit.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "deck-identifier-path-shape",
        .default_level = severity::warning,
        .area = "ids",
        .needs = phase::document,
        .spec_ref = "DECK.md#3.3",
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
        .spec_ref = "DECK.md#4.1.6; DECK.md#9.4",
        .explanation = "Under artwork_complete, a deck level descriptor is above every value the "
                       "deck's artwork "
                       "carries for it. The deck says the content is somewhere in it and the "
                       "annotation says it is "
                       "nowhere, so one of the two is unfinished.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "declared-card-without-image",
        .default_level = severity::error,
        .area = "cards",
        .needs = phase::filesystem,
        .spec_ref = "DECK.md#4.3; DECK.md#9.4",
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
        .spec_ref = "DECK.md#appendix-b; DECK.md#appendix-e",
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
        .spec_ref = "DECK.md#4.3.2; DECK.md#9.4",
        .explanation =
            "Two major arcana declare the same position. Ordering stays well defined, since ties "
            "break by key, but the resulting order is not the one either declaration asked for.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "duplicate-chain-extension",
        .default_level = severity::warning,
        .area = "images",
        .needs = phase::filesystem,
        .spec_ref = "DECK.md#5.7.4; DECK.md#9.4",
        .explanation = "Two files in one directory share a stem and carry two different "
                       "extension-chain formats. Resolution is well defined, but one of the two is "
                       "usually a conversion tool's leftover.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "duplicate-deck-identifier",
        .default_level = severity::warning,
        .area = "ids",
        .needs = phase::library,
        .spec_ref = "DECK.md#2.2.3; DECK.md#3.4; DECK.md#9.4",
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
        .spec_ref = "DECK.md#4.4; DECK.md#9.4",
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
        .spec_ref = "DECK.md#4.3.1; DECK.md#9.4",
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
        .spec_ref = "DECK.md#4.5; DECK.md#9.4",
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
        .spec_ref = "DECK.md#4.5; DECK.md#9.4",
        .explanation =
            "A card listed as excluded ships artwork anyway. Discovery reads the files rather than "
            "the declaration, so the card resolves. Remove it from the list, or remove its files.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "follows-self",
        .default_level = severity::error,
        .area = "ids",
        .needs = phase::document,
        .spec_ref = "DECK.md#4.1.3; DECK.md#9.4",
        .explanation =
            "The follows field names this deck's own identifier or the deck it signifies. A deck "
            "is not "
            "patterned on itself, and following carries none of signifies' merge semantics.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "ignored-card-back-file",
        .default_level = severity::warning,
        .area = "backs",
        .needs = phase::filesystem,
        .spec_ref = "DECK.md#5.5; DECK.md#5.7.2; DECK.md#9.4",
        .explanation = "This file in a card back directory defines no design: its stem is not a "
                       "custom name, or its extension is outside the chain. Rename it, or point a "
                       "design's image path at it.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "ignored-image-root-lookalike",
        .default_level = severity::info,
        .area = "images",
        .needs = phase::filesystem,
        .spec_ref = "DECK.md#5.7.1",
        .explanation = "This top-level directory nearly matches the image root pattern but is not "
                       "one, so discovery ignores it and its contents are not cards. It holds a "
                       "major or minor arcana subtree.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "ignored-key-on-variant",
        .default_level = severity::warning,
        .area = "cards",
        .needs = phase::document,
        .spec_ref = "DECK.md#4.3; DECK.md#9.4",
        .explanation = "A number, position or default_variant is declared on a variant reference "
                       "key, where an "
                       "application ignores all three. They belong to the card rather than to one "
                       "of its artworks.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "language-tag-case-collision",
        .default_level = severity::error,
        .area = "names",
        .needs = phase::filesystem,
        .spec_ref = "DECK.md#6.1; DECK.md#9.4",
        .explanation = "Two name files carry tags differing only in case. Applications compare "
                       "tags case-insensitively, so the deck resolves to one file on one platform "
                       "and the other elsewhere.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "malformed-deck-toml",
        .default_level = severity::error,
        .area = "deck",
        .needs = phase::filesystem,
        .spec_ref = "DECK.md#2.3; DECK.md#9.4",
        .explanation = "The deck's deck.toml is not well-formed TOML 1.0.0.",
        .applies_to = {.min = 1, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "malformed-name-file",
        .default_level = severity::error,
        .area = "names",
        .needs = phase::filesystem,
        .spec_ref = "DECK.md#2.3",
        .explanation = "This name file is not well-formed TOML 1.0.0.",
        .applies_to = {.min = 1, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "malformed-surrogate-file",
        .default_level = severity::error,
        .area = "surrogate",
        .needs = phase::filesystem,
        .spec_ref = "DECK.md#5.8.1; DECK.md#9.4",
        .explanation = "This surrogate file is not well-formed TOML 1.0.0.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "missing-alt-text",
        .default_level = severity::warning,
        .area = "names",
        .needs = phase::filesystem,
        .spec_ref = "DECK.md#6.4; DECK.md#9.4",
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
        .spec_ref = "DECK.md#4.1.6; DECK.md#9.4",
        .explanation = "An artwork declares a content rating descriptor for a system whose deck "
                       "level subtable "
                       "carries no artwork_complete. Without it an application cannot tell an "
                       "unannotated artwork "
                       "from an unrated one.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "missing-card-back-image",
        .default_level = severity::error,
        .area = "backs",
        .needs = phase::filesystem,
        .spec_ref = "DECK.md#4.2; DECK.md#9.4",
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
        .spec_ref = "DECK.md#2.4; DECK.md#9.4",
        .explanation = "A container's first entry is not an uncompressed mimetype file carrying "
                       "the media type. "
                       "Applications still read it, but it cannot be recognised by its leading "
                       "bytes and presents as "
                       "a plain archive.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "missing-deck-identifier",
        .default_level = severity::warning,
        .area = "ids",
        .needs = phase::document,
        .spec_ref = "DECK.md#3.4; DECK.md#9.4",
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
        .spec_ref = "DECK.md#2.3; DECK.md#9.4",
        .explanation = "This directory contains no deck.toml and is not a deck at all.",
        .applies_to = {.min = 1, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "missing-default-language-file",
        .default_level = severity::error,
        .area = "names",
        .needs = phase::filesystem,
        .spec_ref = "DECK.md#6.1; DECK.md#9.4",
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
        .spec_ref = "DECK.md#7.2; DECK.md#9.4",
        .explanation = "A license_files entry names a file the deck does not ship.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "missing-license-text",
        .default_level = severity::warning,
        .area = "deck",
        .needs = phase::filesystem,
        .spec_ref = "DECK.md#7.2",
        .explanation = "The deck declares a license but ships no license text. An SPDX expression "
                       "names terms without conveying them. List the text under license_files, or "
                       "put a LICENSE file at the deck root.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "missing-packager",
        .default_level = severity::warning,
        .area = "deck",
        .needs = phase::document,
        .spec_ref = "DECK.md#7.6; DECK.md#9.4",
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
        .spec_ref = "DECK.md#4; DECK.md#9.4",
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
        .spec_ref = "DECK.md#4.3; DECK.md#9.4",
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
        .spec_ref = "DECK.md#7; DECK.md#9.4",
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
        .spec_ref = "DECK.md#3.1.2; DECK.md#4.5",
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
        .spec_ref = "DECK.md#6.1",
        .explanation = "This language tag is well-formed but not canonical. Rename the file.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "non-utf8-name-file",
        .default_level = severity::error,
        .area = "names",
        .needs = phase::filesystem,
        .spec_ref = "DECK.md#2.3",
        .explanation = "This name file is not encoded as UTF-8.",
        .applies_to = {.min = 1, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "non-utf8-toml",
        .default_level = severity::error,
        .area = "deck",
        .needs = phase::filesystem,
        .spec_ref = "DECK.md#2.3",
        .explanation = "This deck.toml is not encoded as UTF-8. TOML 1.0.0 defines no other "
                       "encoding, so the file is not a TOML document whatever its bytes parse as.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "packager-equals-author",
        .default_level = severity::warning,
        .area = "deck",
        .needs = phase::document,
        .spec_ref = "DECK.md#7.6; DECK.md#9.4",
        .explanation =
            "The packager and author fields carry the same value. The field exists to "
            "distinguish whoever assembled the directory from whoever created the artwork. "
            "Drop it, or correct whichever is wrong.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "palette-snapped-length-mismatch",
        .default_level = severity::warning,
        .area = "surrogate",
        .needs = phase::filesystem,
        .spec_ref = "DECK.md#5.8.1; DECK.md#9.4",
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
        .spec_ref = "DECK.md#6.2; DECK.md#6.4; DECK.md#9.4",
        .explanation = "This name file gives alt text to some entities of a kind and not to this "
                       "one. The two facets "
                       "are written as separate blocks, so an entity missed out of one is "
                       "invisible to a reader "
                       "checking the other.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "position-on-minor-arcanum",
        .default_level = severity::warning,
        .area = "cards",
        .needs = phase::document,
        .spec_ref = "DECK.md#4.3; DECK.md#9.4",
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
        .spec_ref = "DECK.md#4.4; DECK.md#9.4",
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
        .spec_ref = "DECK.md#5.3",
        .explanation = "This raster image is not under a height-named image root and is therefore "
                       "not a card asset. Discovery ignores it, and it will never be shown.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "redistribution-contradicts-rights-status",
        .default_level = severity::warning,
        .area = "deck",
        .needs = phase::document,
        .spec_ref = "DECK.md#7.5; DECK.md#9.4",
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
        .spec_ref = "DECK.md#7.5; DECK.md#9.4",
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
        .spec_ref = "DECK.md#3.2; DECK.md#3.5; DECK.md#9.4",
        .explanation =
            "A key the author coined is one of the reserved canonical names: major_arcana, "
            "minor_arcana, the four canonical suits, or the fourteen canonical ranks.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "signifies-self",
        .default_level = severity::error,
        .area = "ids",
        .needs = phase::document,
        .spec_ref = "DECK.md#4.1.2; DECK.md#9.4",
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
        .spec_ref = "DECK.md#2.3; DECK.md#9.4",
        .explanation = "Two files in one directory have stems differing only in case. Applications "
                       "compare stems case-insensitively, so this is one name with two files "
                       "behind it. Rename one of the two.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "surrogate-deck-redistribution-full",
        .default_level = severity::warning,
        .area = "surrogate",
        .needs = phase::filesystem,
        .spec_ref = "DECK.md#5.9; DECK.md#9.4",
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
        .spec_ref = "DECK.md#5.9; DECK.md#9.4",
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
        .spec_ref = "DECK.md#5.9; DECK.md#9.4",
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
        .spec_ref = "DECK.md#5.9; DECK.md#9.4",
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
        .spec_ref = "DECK.md#5.2",
        .explanation = "This SVG is not under the scalable directory and is therefore not a card "
                       "asset. Discovery ignores it, and it will never be shown.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "symlink-escapes-deck-root",
        .default_level = severity::error,
        .area = "deck",
        .needs = phase::filesystem,
        .spec_ref = "DECK.md#10.1",
        .explanation = "A symbolic link inside the deck leads outside the deck root. A deck "
                       "arrives from outside the system, so this reads a file its author chose on "
                       "a machine they do not own.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "unknown-artwork-rating-system",
        .default_level = severity::error,
        .area = "deck",
        .needs = phase::document,
        .spec_ref = "DECK.md#4.1.6; DECK.md#9.4",
        .explanation =
            "An artwork is rated under a system the deck level content rating table does not "
            "declare. An "
            "artwork refines the deck's declaration and cannot introduce a system of its own.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "unknown-default-card-back",
        .default_level = severity::error,
        .area = "backs",
        .needs = phase::filesystem,
        .spec_ref = "DECK.md#4.2; DECK.md#9.4",
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
        .spec_ref = "DECK.md#6.2.1; DECK.md#9.4",
        .explanation = "A key in a name file's alt text metadata subtable is not one the metadata "
                       "table defines. The subtable is an override, so the key has nothing to "
                       "override and is ignored.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "unknown-name-entity-kind",
        .default_level = severity::error,
        .area = "names",
        .needs = phase::filesystem,
        .spec_ref = "DECK.md#6.2; DECK.md#9.4",
        .explanation = "A table below a name file's facet names no entity kind this specification "
                       "defines. The kinds "
                       "are closed: card, suit, rank, card_back, variant and group, each written "
                       "in the singular.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "unknown-name-facet",
        .default_level = severity::error,
        .area = "names",
        .needs = phase::filesystem,
        .spec_ref = "DECK.md#6.2; DECK.md#9.4",
        .explanation =
            "A top-level table in this name file is neither the name facet, the alt_text facet nor "
            "the "
            "reserved metadata table. An unrecognised facet supplies no strings at all, so a fully "
            "translated deck would present as an untranslated one.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "unknown-name-key",
        .default_level = severity::error,
        .area = "names",
        .needs = phase::filesystem,
        .spec_ref = "DECK.md#6.2; DECK.md#6.2.2; DECK.md#9.4",
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
        .spec_ref = "DECK.md#5.8.1; DECK.md#9.4",
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
        .spec_ref = "DECK.md#4; DECK.md#8",
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
        .spec_ref = "DECK.md#4.3; DECK.md#9.4",
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
        .spec_ref = "DECK.md#4.2; DECK.md#4.3",
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
        .spec_ref = "DECK.md#6.3; DECK.md#9.4",
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
        .spec_ref = "DECK.md#4.1.6; DECK.md#9.4",
        .explanation =
            "A content rating system is outside the registry and is not prefixed. Applications "
            "ignore it, "
            "and a later version of the specification may claim the name. Prefix it with x_.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "unregistered-link-rel",
        .default_level = severity::warning,
        .area = "deck",
        .needs = phase::document,
        .spec_ref = "DECK.md#4.1.1; DECK.md#9.4",
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
        .spec_ref = "DECK.md#4.1.5; DECK.md#9.4",
        .explanation = "A product_ids scheme is outside the registry of isbn, gtin and "
                       "publisher_sku and is not "
                       "prefixed. Applications ignore it, and a later version of the specification "
                       "may claim the "
                       "name. Prefix it with x_.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "unsafe-container-entry-name",
        .default_level = severity::error,
        .area = "container",
        .needs = phase::library,
        .spec_ref = "DECK.md#2.4; DECK.md#9.4",
        .explanation = "A container entry name is absolute, names a drive, carries a dot-dot, dot "
                       "or empty segment, "
                       "or repeats another entry. A validator checks entry names itself, because "
                       "archive libraries "
                       "repair them by default.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "unsafe-path",
        .default_level = severity::error,
        .area = "deck",
        .needs = phase::filesystem,
        .spec_ref = "DECK.md#2.3; DECK.md#10.1; DECK.md#9.4",
        .explanation = "A path in deck.toml begins with a slash, contains a parent-directory "
                       "segment, or resolves outside the deck root. Applications must reject such "
                       "a path rather than resolve it.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "unused-artwork-complete",
        .default_level = severity::warning,
        .area = "deck",
        .needs = phase::document,
        .spec_ref = "DECK.md#4.1.6; DECK.md#9.4",
        .explanation = "An artwork_complete is declared on a system no artwork carries a "
                       "descriptor for, so the key "
                       "describes an annotation that does not exist.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "variant-card-without-default",
        .default_level = severity::error,
        .area = "cards",
        .needs = phase::filesystem,
        .spec_ref = "DECK.md#4.3; DECK.md#5.7.5; DECK.md#9.4",
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
        .spec_ref = "DECK.md#4.3; DECK.md#9.4",
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
        .spec_ref = "DECK.md#4; DECK.md#9.4",
        .explanation = "A key carries a value of a type other than the one its field table gives. "
                       "The usual case is a date: created_date and updated_date are strings, so an "
                       "unquoted date is wrong. Quote it.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
};

// Strictly ascending proves sortedness and uniqueness in one pass, which is
// what lets find_rule() binary-search.
consteval bool catalogue_is_sorted_and_unique()
{
    for (std::size_t i = 1; i < catalogue.size(); ++i)
        if (!(catalogue[i - 1].code < catalogue[i].code))
            return false;

    return true;
}

static_assert(
    catalogue_is_sorted_and_unique(),
    "the catalogue must be sorted strictly ascending by code, with no duplicate code"
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

}  // namespace

std::span<rule const> all_rules() noexcept
{
    return catalogue;
}

rule const* lookup(std::string_view code) noexcept
{
    auto const* const found = std::ranges::lower_bound(catalogue, code, {}, &rule::code);
    if (found == catalogue.end() || found->code != code)
        return nullptr;

    return &*found;
}

}  // namespace arcana::validation
