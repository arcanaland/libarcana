// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

// The diagnostic catalogue.
//
// Derived from the Tarot Deck Specification v2

#include <arcana/validation.hpp>

#include "data/ascii.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <format>
#include <fstream>
#include <ios>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <tuple>
#include <utility>
#include <vector>

namespace arcana
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
        .code = "bad-cards-table-key",
        .default_level = severity::error,
        .area = "ids",
        .needs = phase::document,
        .spec_ref = "DECK.md#3.1; DECK.md#4.3; DECK.md#9.4",
        .explanation =
            "A key in the cards table is not a canonical ID. A major arcanum's key carries both of "
            "its digits, and a variant reference belongs in the card variants table instead.",
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
        .spec_ref = "DECK.md#3.3; DECK.md#9.4",
        .explanation = "The deck's identifier is not a well-formed qualified identifier: a realm, "
                       "a slash, one or more path segments, and an optional fragment after a hash.",
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
            "The signifies field is not a well-formed qualified identifier. The value is used as a "
            "merge key against another package's identifier, so a malformed one can never match.",
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
        .spec_ref = "DECK.md#5.5; DECK.md#9.4",
        .explanation = "A card back design is supplied in neither PNG nor JPEG. Backs have no "
                       "reference deck to fall back on, so an application that cannot decode the "
                       "design substitutes a generic back.",
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
        .spec_ref = "DECK.md#appendix-b; DECK.md#appendix-d",
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
        .code = "missing-edition-default",
        .default_level = severity::error,
        .area = "cards",
        .needs = phase::document,
        .spec_ref = "DECK.md#4.6; DECK.md#9.4",
        .explanation =
            "The deck defines more than one edition and the editions table declares no default. "
            "The field may be omitted only where exactly one edition is defined.",
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
        .spec_ref = "DECK.md#4.7; DECK.md#9.4",
        .explanation =
            "A card variant's image path points at no file, which leaves the variant unresolvable. "
            "Correct the path, or drop it and name the file by the convention.",
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
        .spec_ref = "DECK.md#3.1",
        .explanation = "This card reference is not a canonical ID with an optional variant suffix.",
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
        .code = "unknown-edition-card-back",
        .default_level = severity::error,
        .area = "backs",
        .needs = phase::filesystem,
        .spec_ref = "DECK.md#4.6; DECK.md#9.4",
        .explanation = "An edition's card_back names no design the deck has, which leaves the "
                       "edition indistinguishable from the default printing. Omit the field to "
                       "take the deck's own default.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "unknown-edition-default",
        .default_level = severity::error,
        .area = "cards",
        .needs = phase::document,
        .spec_ref = "DECK.md#4.6; DECK.md#9.4",
        .explanation =
            "The default declared under the editions table names no edition the deck defines, so "
            "the edition an application presents where the user has not chosen is undefined.",
        .applies_to = {.min = 2, .max = 2},
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
        .code = "unknown-name-key",
        .default_level = severity::error,
        .area = "names",
        .needs = phase::filesystem,
        .spec_ref = "DECK.md#6.2; DECK.md#9.4",
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
        .spec_ref = "DECK.md#4.7; DECK.md#9.4",
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
        .code = "variant-card-without-default",
        .default_level = severity::error,
        .area = "cards",
        .needs = phase::filesystem,
        .spec_ref = "DECK.md#4.7; DECK.md#5.7.6; DECK.md#9.4",
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
        .spec_ref = "DECK.md#4.7; DECK.md#9.4",
        .explanation = "A card variants table is keyed on a card the deck does not have, so it "
                       "annotates nothing. A card exists because the deck ships an asset for it.",
        .applies_to = {.min = 2, .max = 2},
        .experimental = false,
    },
    rule{
        .code = "variant-missing-alt-text",
        .default_level = severity::warning,
        .area = "names",
        .needs = phase::filesystem,
        .spec_ref = "DECK.md#4.7; DECK.md#6.4",
        .explanation = "A card variant carries no alt text of its own and falls back to the "
                       "card's, which omits the very difference the variant exists for.",
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


// The major a deck is judged under when [deck].schema_version cannot be read:
// the newest this catalogue knows.
constexpr std::uint8_t current_schema_major = 2;

// ---------------------------------------------------------------------------
// The deck's tree
// ---------------------------------------------------------------------------

// One regular file inside a deck root.
struct deck_file
{
    // Deck-root-relative. This is what a diagnostic's `path` carries.
    std::filesystem::path relative;

    std::filesystem::path absolute;
};

// Every regular file under the deck root, sorted by relative path.
//
// Walked once and shared by every phase::filesystem check, so no check pays
// for the traversal twice and every check sees the same order regardless of
// what directory_iterator hands back.
//
// Directory symlinks are not descended and no symlink resolving outside the
// deck root is followed (DECK.md section 10.1). Escaping links are skipped
// silently here; symlink-escapes-deck-root is what reports them.
std::vector<deck_file> walk_deck(std::filesystem::path const& root)
{
    namespace fs = std::filesystem;

    std::vector<deck_file> files;

    std::error_code ec;
    if (!fs::is_directory(root, ec))
        return files;

    auto const canonical_root = fs::weakly_canonical(root, ec);
    if (ec)
        return files;

    fs::recursive_directory_iterator it{root, fs::directory_options::skip_permission_denied, ec};
    if (ec)
        return files;

    // True when what this path names resolves to the deck root itself or to
    // something under it.
    auto const stays_inside = [&canonical_root](fs::path const& candidate)
    {
        std::error_code resolve_ec;
        auto const resolved = fs::weakly_canonical(candidate, resolve_ec);
        if (resolve_ec)
            return false;

        auto const inside = resolved.lexically_relative(canonical_root);
        return !inside.empty() && *inside.begin() != "..";
    };

    for (fs::recursive_directory_iterator const end; it != end; it.increment(ec))
    {
        if (ec)
            break;

        auto const& path = it->path();

        if (it->is_symlink(ec) && !stays_inside(path))
        {
            it.disable_recursion_pending();
            continue;
        }

        if (!it->is_regular_file(ec))
            continue;

        files.push_back({.relative = path.lexically_relative(root), .absolute = path});
    }

    std::ranges::sort(files, {}, &deck_file::relative);
    return files;
}

// ---------------------------------------------------------------------------
// What a check is
// ---------------------------------------------------------------------------

// What a check reports. Every field but the message is optional because most
// rules locate their finding with only one of them.
struct finding
{
    std::string message;
    std::optional<std::string> card;
    std::optional<std::filesystem::path> path;
    std::optional<std::string> key;
};

// What a check sees. It never sees anything else: no globals, no caching, no
// logging, no locale.
struct check_context
{
    deck const& d;

    // The deck's own tree, already walked. Empty for a phase::document check
    // whose deck has no root on disk.
    std::span<deck_file const> files;

    // The rule being run. A finding takes its level and code from here.
    rule const& r;

    std::vector<diagnostic>& out;

    void report(finding what) const
    {
        out.push_back(
            diagnostic{
                .level = r.default_level,
                .code = r.code,
                .message = std::move(what.message),
                .card = std::move(what.card),
                .path = std::move(what.path),
                .key = std::move(what.key),
            }
        );
    }
};

using check_fn = void (*)(check_context const&);

// ---------------------------------------------------------------------------
// Checks: ansi
// ---------------------------------------------------------------------------

// DECK.md section 5.7.1: an ANSI image root is `ansi<lines>/`, where <lines> is
// a decimal integer greater than zero written without a sign, leading zeroes or
// separators. `ansi/`, `ansi0/` and `ansi032/` are therefore ordinary
// directories that discovery ignores.
//
// src/loader/loader.cpp's looks_like_ansi_root() is looser than this: it admits
// leading zeroes. That divergence is a loader bug and is not fixed here.
bool is_ansi_root_name(std::string_view name)
{
    if (!name.starts_with("ansi"))
        return false;

    auto const lines = name.substr(4);
    if (lines.empty() || lines.front() == '0')
        return false;

    return std::ranges::all_of(lines, data::is_digit);
}

// How much of a file is read before giving up on finding an ESC. ANSI art is
// small; anything this far in without one is not art that a terminal reads.
constexpr std::size_t ansi_sniff_bytes = 64UL * 1024UL;

// How much is read at a time while doing it.
constexpr std::size_t ansi_sniff_chunk = 4096;

// DECK.md section 5.4 fixes an ANSI file's kind by its content rather than its
// extension: art carrying escape sequences necessarily contains ESC (0x1B), and
// a file with no ESC byte is plain text.
//
// Plain text is indistinguishable from any other text file, so this recognizes
// escape-carrying art only. It under-reports rather than guessing, which is why
// the rule this serves is info and not error. A NUL byte before any ESC settles
// the file as binary, which keeps card artwork out of the result.
bool carries_ansi_escapes(std::filesystem::path const& file)
{
    std::ifstream stream{file, std::ios::binary};
    if (!stream)
        return false;

    std::array<char, ansi_sniff_chunk> buffer{};
    for (std::size_t seen = 0; seen < ansi_sniff_bytes;)
    {
        stream.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));

        auto const got = static_cast<std::size_t>(stream.gcount());
        if (got == 0)
            break;

        seen += got;

        auto const chunk = std::string_view(buffer.data(), got);
        auto const stop = chunk.find_first_of(std::string_view("\0\x1b", 2));
        if (stop != std::string_view::npos)
            return chunk[stop] == '\x1b';
    }

    return false;
}

void check_ansi_outside_image_root(check_context const& ctx)
{
    for (auto const& file : ctx.files)
    {
        if (is_ansi_root_name(file.relative.begin()->string()))
            continue;

        if (!carries_ansi_escapes(file.absolute))
            continue;

        auto const shown = file.relative.generic_string();
        ctx.report({
            .message =
                std::format("'{}' carries ANSI escapes but is under no ansi<lines>/ root", shown),
            .path = file.relative,
        });
    }
}

// ---------------------------------------------------------------------------
// The dispatch table
// ---------------------------------------------------------------------------

// TASK-016 defers two rules, both for a reason that outlives the task.
// aspect-ratio-mismatch needs an image decoder, and no image decoder enters
// this tree; duplicate-deck-identifier is the sole phase::library rule and
// validate(deck const&) cannot reach a sibling deck. Their catalogue entries
// are untouched, and the coverage test names them.
constexpr check_fn deferred = nullptr;

// No check is written yet. TASK-016 layers 2-8 replace these one area at a
// time; each layer also deletes its codes from the coverage test's
// not_yet_covered list.
constexpr check_fn pending = nullptr;

struct check_entry
{
    std::string_view code;
    check_fn run;
};

// One row per catalogue rule, in catalogue order. The static_assert below ties
// the two element by element, so a rule minted without a row here -- or a row
// whose code no longer names a rule -- is a compile error rather than a check
// that silently never runs.
constexpr std::array checks{
    check_entry{.code = "ansi-outside-image-root", .run = check_ansi_outside_image_root},
    check_entry{.code = "aspect-ratio-mismatch", .run = deferred},
    check_entry{.code = "backslash-in-path", .run = pending},
    check_entry{.code = "bad-app-realm", .run = pending},
    check_entry{.code = "bad-card-back-design-key", .run = pending},
    check_entry{.code = "bad-cards-table-key", .run = pending},
    check_entry{.code = "bad-custom-name", .run = pending},
    check_entry{.code = "bad-deck-identifier", .run = pending},
    check_entry{.code = "bad-language-tag", .run = pending},
    check_entry{.code = "bad-link-rel", .run = pending},
    check_entry{.code = "bad-link-url", .run = pending},
    check_entry{.code = "bad-name-template-placeholder", .run = pending},
    check_entry{.code = "bad-palette-color", .run = pending},
    check_entry{.code = "bad-palette-snapped-color", .run = pending},
    check_entry{.code = "bad-rights-field-value", .run = pending},
    check_entry{.code = "bad-rights-status-uri", .run = pending},
    check_entry{.code = "bad-schema-version", .run = pending},
    check_entry{.code = "bad-signifies", .run = pending},
    check_entry{.code = "bad-spdx-expression", .run = pending},
    check_entry{.code = "bom-in-toml", .run = pending},
    check_entry{.code = "card-back-default-by-collation", .run = pending},
    check_entry{.code = "card-back-not-baseline-format", .run = pending},
    check_entry{.code = "deck-has-no-cards", .run = pending},
    check_entry{.code = "deck-identifier-path-shape", .run = pending},
    check_entry{.code = "declared-card-without-image", .run = pending},
    check_entry{.code = "deprecated-1-0-key", .run = pending},
    check_entry{.code = "duplicate-card-position", .run = pending},
    check_entry{.code = "duplicate-chain-extension", .run = pending},
    check_entry{.code = "duplicate-deck-identifier", .run = deferred},
    check_entry{.code = "duplicate-rank-in-ranks", .run = pending},
    check_entry{.code = "empty-card-number", .run = pending},
    check_entry{.code = "excluded-card-also-declared", .run = pending},
    check_entry{.code = "excluded-card-has-image", .run = pending},
    check_entry{.code = "ignored-card-back-file", .run = pending},
    check_entry{.code = "ignored-image-root-lookalike", .run = pending},
    check_entry{.code = "language-tag-case-collision", .run = pending},
    check_entry{.code = "malformed-deck-toml", .run = pending},
    check_entry{.code = "malformed-name-file", .run = pending},
    check_entry{.code = "malformed-surrogate-file", .run = pending},
    check_entry{.code = "missing-alt-text", .run = pending},
    check_entry{.code = "missing-card-back-image", .run = pending},
    check_entry{.code = "missing-deck-identifier", .run = pending},
    check_entry{.code = "missing-deck-toml", .run = pending},
    check_entry{.code = "missing-default-language-file", .run = pending},
    check_entry{.code = "missing-edition-default", .run = pending},
    check_entry{.code = "missing-license-file", .run = pending},
    check_entry{.code = "missing-license-text", .run = pending},
    check_entry{.code = "missing-packager", .run = pending},
    check_entry{.code = "missing-required-field", .run = pending},
    check_entry{.code = "missing-variant-image", .run = pending},
    check_entry{.code = "no-rights-statement", .run = pending},
    check_entry{.code = "non-canonical-card-reference", .run = pending},
    check_entry{.code = "non-canonical-language-tag", .run = pending},
    check_entry{.code = "non-utf8-name-file", .run = pending},
    check_entry{.code = "non-utf8-toml", .run = pending},
    check_entry{.code = "packager-equals-author", .run = pending},
    check_entry{.code = "palette-snapped-length-mismatch", .run = pending},
    check_entry{.code = "position-on-minor-arcanum", .run = pending},
    check_entry{.code = "rank-without-image", .run = pending},
    check_entry{.code = "raster-outside-image-root", .run = pending},
    check_entry{.code = "redistribution-contradicts-rights-status", .run = pending},
    check_entry{.code = "redistribution-narrower-than-license", .run = pending},
    check_entry{.code = "reserved-custom-name", .run = pending},
    check_entry{.code = "signifies-self", .run = pending},
    check_entry{.code = "stem-case-collision", .run = pending},
    check_entry{.code = "surrogate-deck-redistribution-full", .run = pending},
    check_entry{.code = "surrogate-deck-without-buy-link", .run = pending},
    check_entry{.code = "surrogate-deck-without-license", .run = pending},
    check_entry{.code = "surrogate-deck-without-signifies", .run = pending},
    check_entry{.code = "svg-outside-scalable", .run = pending},
    check_entry{.code = "symlink-escapes-deck-root", .run = pending},
    check_entry{.code = "unknown-default-card-back", .run = pending},
    check_entry{.code = "unknown-edition-card-back", .run = pending},
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
    checks_cover_catalogue(), "every rule needs a row in the dispatch table, in catalogue order"
);

}  // namespace

std::span<rule const> rules() noexcept
{
    return catalogue;
}

rule const* find_rule(std::string_view code) noexcept
{
    auto const* const found = std::ranges::lower_bound(catalogue, code, {}, &rule::code);
    if (found == catalogue.end() || found->code != code)
        return nullptr;

    return &*found;
}

std::vector<diagnostic> validate(deck const& d)
{
    // A deck whose schema_version cannot be read is judged under the current
    // major. bad-schema-version is what reports the field itself.
    auto const major = schema_major(d.metadata).value_or(current_schema_major);

    auto const files = walk_deck(d.root_path);

    std::vector<diagnostic> found;
    for (std::size_t i = 0; i < catalogue.size(); ++i)
    {
        rule const& r = catalogue[i];

        // A phase::library rule needs sibling decks, which this overload does
        // not have. Nothing here can run one.
        if (r.needs == phase::library)
            continue;

        if (!r.applies_to.contains(major))
            continue;

        if (checks[i].run == nullptr)
            continue;

        check_context const ctx{.d = d, .files = files, .r = r, .out = found};
        checks[i].run(ctx);
    }

    // Ascending by (code, card, path, key), disengaged optionals first, with
    // the message breaking a tie so that two findings of one rule about one
    // place still come back in a fixed order.
    std::ranges::sort(
        found,
        [](diagnostic const& left, diagnostic const& right)
        {
            return std::tie(left.code, left.card, left.path, left.key, left.message) <
                   std::tie(right.code, right.card, right.path, right.key, right.message);
        }
    );

    return found;
}

}  // namespace arcana
