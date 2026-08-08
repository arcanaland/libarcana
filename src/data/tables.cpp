// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include "tables.hpp"

#include "ascii.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <optional>
#include <string_view>

namespace arcana::data
{

bool is_css_color_name(std::string_view name) noexcept
{
    // CSS Color 4 section 6.1.
    constexpr auto names = std::to_array<std::string_view>(
        {"aliceblue",
         "antiquewhite",
         "aqua",
         "aquamarine",
         "azure",
         "beige",
         "bisque",
         "black",
         "blanchedalmond",
         "blue",
         "blueviolet",
         "brown",
         "burlywood",
         "cadetblue",
         "chartreuse",
         "chocolate",
         "coral",
         "cornflowerblue",
         "cornsilk",
         "crimson",
         "cyan",
         "darkblue",
         "darkcyan",
         "darkgoldenrod",
         "darkgray",
         "darkgreen",
         "darkgrey",
         "darkkhaki",
         "darkmagenta",
         "darkolivegreen",
         "darkorange",
         "darkorchid",
         "darkred",
         "darksalmon",
         "darkseagreen",
         "darkslateblue",
         "darkslategray",
         "darkslategrey",
         "darkturquoise",
         "darkviolet",
         "deeppink",
         "deepskyblue",
         "dimgray",
         "dimgrey",
         "dodgerblue",
         "firebrick",
         "floralwhite",
         "forestgreen",
         "fuchsia",
         "gainsboro",
         "ghostwhite",
         "gold",
         "goldenrod",
         "gray",
         "green",
         "greenyellow",
         "grey",
         "honeydew",
         "hotpink",
         "indianred",
         "indigo",
         "ivory",
         "khaki",
         "lavender",
         "lavenderblush",
         "lawngreen",
         "lemonchiffon",
         "lightblue",
         "lightcoral",
         "lightcyan",
         "lightgoldenrodyellow",
         "lightgray",
         "lightgreen",
         "lightgrey",
         "lightpink",
         "lightsalmon",
         "lightseagreen",
         "lightskyblue",
         "lightslategray",
         "lightslategrey",
         "lightsteelblue",
         "lightyellow",
         "lime",
         "limegreen",
         "linen",
         "magenta",
         "maroon",
         "mediumaquamarine",
         "mediumblue",
         "mediumorchid",
         "mediumpurple",
         "mediumseagreen",
         "mediumslateblue",
         "mediumspringgreen",
         "mediumturquoise",
         "mediumvioletred",
         "midnightblue",
         "mintcream",
         "mistyrose",
         "moccasin",
         "navajowhite",
         "navy",
         "oldlace",
         "olive",
         "olivedrab",
         "orange",
         "orangered",
         "orchid",
         "palegoldenrod",
         "palegreen",
         "paleturquoise",
         "palevioletred",
         "papayawhip",
         "peachpuff",
         "peru",
         "pink",
         "plum",
         "powderblue",
         "purple",
         "rebeccapurple",
         "red",
         "rosybrown",
         "royalblue",
         "saddlebrown",
         "salmon",
         "sandybrown",
         "seagreen",
         "seashell",
         "sienna",
         "silver",
         "skyblue",
         "slateblue",
         "slategray",
         "slategrey",
         "snow",
         "springgreen",
         "steelblue",
         "tan",
         "teal",
         "thistle",
         "tomato",
         "turquoise",
         "violet",
         "wheat",
         "white",
         "whitesmoke",
         "yellow",
         "yellowgreen"}
    );

    constexpr std::size_t css_color_4_names = 148;

    static_assert(names.size() == css_color_4_names, "CSS Color 4 names exactly 148 colours");
    static_assert(std::ranges::is_sorted(names), "binary_search needs a sorted table");

    return std::ranges::binary_search(names, name);
}

bool is_srgb_hex_triplet(std::string_view s) noexcept
{
    constexpr std::size_t triplet_size = 7;

    return s.size() == triplet_size && s.front() == '#' &&
           std::ranges::all_of(
               s.substr(1), [](char c) { return is_digit(c) || (c >= 'a' && c <= 'f'); }
           );
}

bool is_registered_link_rel(std::string_view rel) noexcept
{
    constexpr auto registry =
        std::to_array<std::string_view>({"artist", "buy", "homepage", "publisher", "source"});

    static_assert(std::ranges::is_sorted(registry), "binary_search needs a sorted table");

    return std::ranges::binary_search(registry, rel);
}

bool is_extension_link_rel(std::string_view rel) noexcept
{
    return rel.starts_with("x_");
}

namespace
{

std::optional<std::string_view> strip_uri_scheme(std::string_view uri) noexcept
{
    constexpr std::string_view secure{"https://"};
    constexpr std::string_view plain{"http://"};

    if (uri.starts_with(secure))
        uri.remove_prefix(secure.size());
    else if (uri.starts_with(plain))
        uri.remove_prefix(plain.size());
    else
        return std::nullopt;

    while (uri.ends_with('/')) uri.remove_suffix(1);

    return uri;
}

// The RightsStatements.org vocabulary
struct rights_statement_row
{
    std::string_view uri;
    rights_status_class status;
};

std::optional<rights_status_class> classify_rights_statement(std::string_view stripped) noexcept
{
    constexpr auto statements = std::to_array<rights_statement_row>(
        {{.uri = "rightsstatements.org/vocab/CNE/1.0", .status = rights_status_class::undetermined},
         {.uri = "rightsstatements.org/vocab/InC-EDU/1.0",
          .status = rights_status_class::in_copyright},
         {.uri = "rightsstatements.org/vocab/InC-NC/1.0",
          .status = rights_status_class::in_copyright},
         {.uri = "rightsstatements.org/vocab/InC-OW-EU/1.0",
          .status = rights_status_class::in_copyright},
         {.uri = "rightsstatements.org/vocab/InC-RUU/1.0",
          .status = rights_status_class::in_copyright},
         {.uri = "rightsstatements.org/vocab/InC/1.0", .status = rights_status_class::in_copyright},
         {.uri = "rightsstatements.org/vocab/NKC/1.0", .status = rights_status_class::no_copyright},
         {.uri = "rightsstatements.org/vocab/NoC-CR/1.0",
          .status = rights_status_class::no_copyright},
         {.uri = "rightsstatements.org/vocab/NoC-NC/1.0",
          .status = rights_status_class::no_copyright},
         {.uri = "rightsstatements.org/vocab/NoC-OKLR/1.0",
          .status = rights_status_class::no_copyright},
         {.uri = "rightsstatements.org/vocab/NoC-US/1.0",
          .status = rights_status_class::no_copyright},
         {.uri = "rightsstatements.org/vocab/UND/1.0", .status = rights_status_class::undetermined}}
    );

    static_assert(
        std::ranges::is_sorted(statements, {}, &rights_statement_row::uri),
        "binary_search needs a sorted table"
    );

    auto const* const found =
        std::ranges::lower_bound(statements, stripped, {}, &rights_statement_row::uri);
    if (found == statements.end() || found->uri != stripped)
        return std::nullopt;

    return found->status;
}

constexpr std::size_t max_uri_segments = 3;

// Splits `path` on slashes.
//
// @returns The number of segments, or 0 where any is empty or there are more
//          than `max_uri_segments` of them.
std::size_t split_uri_path(
    std::string_view path, std::array<std::string_view, max_uri_segments>& out
) noexcept
{
    std::size_t count = 0;

    while (true)
    {
        auto const slash = path.find('/');
        auto const segment = path.substr(0, slash);
        if (segment.empty() || count == max_uri_segments)
            return 0;

        out.at(count) = segment;
        ++count;

        if (slash == std::string_view::npos)
            return count;

        path.remove_prefix(slash + 1);
    }
}

// A Creative Commons version segment: "1.0", "2.5", "4.0".
bool is_cc_version(std::string_view segment) noexcept
{
    return !segment.empty() && is_digit(segment.front()) && is_digit(segment.back()) &&
           std::ranges::all_of(segment, [](char c) { return is_digit(c) || c == '.'; });
}

// A ported licence's jurisdiction segment, as in .../by-sa/3.0/us/.
bool is_cc_jurisdiction(std::string_view segment) noexcept
{
    constexpr std::size_t max_jurisdiction = 16;

    return !segment.empty() && segment.size() <= max_jurisdiction &&
           std::ranges::all_of(segment, [](char c) { return is_lcalpha(c) || c == '-'; });
}

bool is_cc_license_path(std::string_view path) noexcept
{
    constexpr auto codes =
        std::to_array<std::string_view>({"by", "by-nc", "by-nc-nd", "by-nc-sa", "by-nd", "by-sa"});

    static_assert(std::ranges::is_sorted(codes), "binary_search needs a sorted table");

    std::array<std::string_view, max_uri_segments> segments{};
    std::size_t const count = split_uri_path(path, segments);
    if (count < 2)
        return false;

    if (!std::ranges::binary_search(codes, segments.at(0)) || !is_cc_version(segments.at(1)))
        return false;

    return count == 2 || is_cc_jurisdiction(segments.at(2));
}

bool is_cc_public_domain_path(std::string_view path) noexcept
{
    constexpr auto codes = std::to_array<std::string_view>({"certification", "mark", "zero"});

    static_assert(std::ranges::is_sorted(codes), "binary_search needs a sorted table");

    std::array<std::string_view, max_uri_segments> segments{};
    std::size_t const count = split_uri_path(path, segments);
    if (count < 1 || count > 2)
        return false;

    if (!std::ranges::binary_search(codes, segments.at(0)))
        return false;

    return count == 1 || is_cc_version(segments.at(1));
}

std::optional<rights_status_class> classify_creative_commons(std::string_view stripped) noexcept
{
    constexpr std::string_view host{"creativecommons.org/"};
    constexpr std::string_view licenses{"licenses/"};
    constexpr std::string_view public_domain{"publicdomain/"};

    if (!stripped.starts_with(host))
        return std::nullopt;

    auto const path = stripped.substr(host.size());

    if (path.starts_with(licenses) && is_cc_license_path(path.substr(licenses.size())))
        return rights_status_class::in_copyright;

    if (path.starts_with(public_domain) &&
        is_cc_public_domain_path(path.substr(public_domain.size())))
        return rights_status_class::no_copyright;

    return std::nullopt;
}

}  // namespace

bool is_rights_status_uri(std::string_view uri) noexcept
{
    return classify_rights_status(uri).has_value();
}

std::optional<rights_status_class> classify_rights_status(std::string_view uri) noexcept
{
    auto const stripped = strip_uri_scheme(uri);
    if (!stripped.has_value())
        return std::nullopt;

    if (auto const statement = classify_rights_statement(*stripped); statement.has_value())
        return statement;

    return classify_creative_commons(*stripped);
}

namespace
{

struct language_subtag_row
{
    std::string_view three;
    std::string_view two;
};

}  // namespace

std::optional<std::string_view> shortest_language_subtag(std::string_view subtag) noexcept
{
    // ISO 639-2, as published by the Library of Congress at
    // https://www.loc.gov/standards/iso639-2/, restricted to the codes that
    // have a two-letter form.
    constexpr auto rows = std::to_array<language_subtag_row>(
        {{.three = "abk", .two = "ab"},   {.three = "afr", .two = "af"},
         {.three = "aka", .two = "ak"},   {.three = "alb", .two = "sq"},
         {.three = "amh", .two = "am"},   {.three = "ara", .two = "ar"},
         {.three = "arg", .two = "an"},   {.three = "arm", .two = "hy"},
         {.three = "asm", .two = "as"},   {.three = "ava", .two = "av"},
         {.three = "ave", .two = "ae"},   {.three = "aym", .two = "ay"},
         {.three = "aze", .two = "az"},   {.three = "bak", .two = "ba"},
         {.three = "bam", .two = "bm"},   {.three = "baq", .two = "eu"},
         {.three = "bel", .two = "be"},   {.three = "ben", .two = "bn"},
         {.three = "bis", .two = "bi"},   {.three = "bod", .two = "bo"},
         {.three = "bos", .two = "bs"},   {.three = "bre", .two = "br"},
         {.three = "bul", .two = "bg"},   {.three = "bur", .two = "my"},
         {.three = "cat", .two = "ca"},   {.three = "ces", .two = "cs"},
         {.three = "cha", .two = "ch"},   {.three = "che", .two = "ce"},
         {.three = "chi", .two = "zh"},   {.three = "chu", .two = "cu"},
         {.three = "chv", .two = "cv"},   {.three = "cor", .two = "kw"},
         {.three = "cos", .two = "co"},   {.three = "cre", .two = "cr"},
         {.three = "cym", .two = "cy"},   {.three = "cze", .two = "cs"},
         {.three = "dan", .two = "da"},   {.three = "deu", .two = "de"},
         {.three = "div", .two = "dv"},   {.three = "dut", .two = "nl"},
         {.three = "dzo", .two = "dz"},   {.three = "ell", .two = "el"},
         {.three = "eng", .two = "en"},   {.three = "epo", .two = "eo"},
         {.three = "est", .two = "et"},   {.three = "eus", .two = "eu"},
         {.three = "ewe", .two = "ee"},   {.three = "fao", .two = "fo"},
         {.three = "fas", .two = "fa"},   {.three = "fij", .two = "fj"},
         {.three = "fin", .two = "fi"},   {.three = "fra", .two = "fr"},
         {.three = "fre", .two = "fr"},   {.three = "fry", .two = "fy"},
         {.three = "ful", .two = "ff"},   {.three = "geo", .two = "ka"},
         {.three = "ger", .two = "de"},   {.three = "gla", .two = "gd"},
         {.three = "gle", .two = "ga"},   {.three = "glg", .two = "gl"},
         {.three = "glv", .two = "gv"},   {.three = "gre", .two = "el"},
         {.three = "grn", .two = "gn"},   {.three = "guj", .two = "gu"},
         {.three = "hat", .two = "ht"},   {.three = "hau", .two = "ha"},
         {.three = "heb", .two = "he"},   {.three = "her", .two = "hz"},
         {.three = "hin", .two = "hi"},   {.three = "hmo", .two = "ho"},
         {.three = "hrv", .two = "hr"},   {.three = "hun", .two = "hu"},
         {.three = "hye", .two = "hy"},   {.three = "ibo", .two = "ig"},
         {.three = "ice", .two = "is"},   {.three = "ido", .two = "io"},
         {.three = "iii", .two = "ii"},   {.three = "iku", .two = "iu"},
         {.three = "ile", .two = "ie"},   {.three = "ina", .two = "ia"},
         {.three = "ind", .two = "id"},   {.three = "ipk", .two = "ik"},
         {.three = "isl", .two = "is"},   {.three = "ita", .two = "it"},
         {.three = "jav", .two = "jv"},   {.three = "jpn", .two = "ja"},
         {.three = "kal", .two = "kl"},   {.three = "kan", .two = "kn"},
         {.three = "kas", .two = "ks"},   {.three = "kat", .two = "ka"},
         {.three = "kau", .two = "kr"},   {.three = "kaz", .two = "kk"},
         {.three = "khm", .two = "km"},   {.three = "kik", .two = "ki"},
         {.three = "kin", .two = "rw"},   {.three = "kir", .two = "ky"},
         {.three = "kom", .two = "kv"},   {.three = "kon", .two = "kg"},
         {.three = "kor", .two = "ko"},   {.three = "kua", .two = "kj"},
         {.three = "kur", .two = "ku"},   {.three = "lao", .two = "lo"},
         {.three = "lat", .two = "la"},   {.three = "lav", .two = "lv"},
         {.three = "lim", .two = "li"},   {.three = "lin", .two = "ln"},
         {.three = "lit", .two = "lt"},   {.three = "ltz", .two = "lb"},
         {.three = "lub", .two = "lu"},   {.three = "lug", .two = "lg"},
         {.three = "mac", .two = "mk"},   {.three = "mah", .two = "mh"},
         {.three = "mal", .two = "ml"},   {.three = "mao", .two = "mi"},
         {.three = "mar", .two = "mr"},   {.three = "may", .two = "ms"},
         {.three = "mkd", .two = "mk"},   {.three = "mlg", .two = "mg"},
         {.three = "mlt", .two = "mt"},   {.three = "mon", .two = "mn"},
         {.three = "mri", .two = "mi"},   {.three = "msa", .two = "ms"},
         {.three = "mya", .two = "my"},   {.three = "nau", .two = "na"},
         {.three = "nav", .two = "nv"},   {.three = "nbl", .two = "nr"},
         {.three = "nde", .two = "nd"},   {.three = "ndo", .two = "ng"},
         {.three = "nep", .two = "ne"},   {.three = "nld", .two = "nl"},
         {.three = "nno", .two = "nn"},   {.three = "nob", .two = "nb"},
         {.three = "nor", .two = "no"},   {.three = "nya", .two = "ny"},
         {.three = "oci", .two = "oc"},   {.three = "oji", .two = "oj"},
         {.three = "ori", .two = "or"},   {.three = "orm", .two = "om"},
         {.three = "oss", .two = "os"},   {.three = "pan", .two = "pa"},
         {.three = "per", .two = "fa"},   {.three = "pli", .two = "pi"},
         {.three = "pol", .two = "pl"},   {.three = "por", .two = "pt"},
         {.three = "pus", .two = "ps"},   {.three = "que", .two = "qu"},
         {.three = "roh", .two = "rm"},   {.three = "ron", .two = "ro"},
         {.three = "rum", .two = "ro"},   {.three = "run", .two = "rn"},
         {.three = "rus", .two = "ru"},   {.three = "sag", .two = "sg"},
         {.three = "san", .two = "sa"},   {.three = "sin", .two = "si"},
         {.three = "slk", .two = "sk"},   {.three = "slo", .two = "sk"},
         {.three = "slv", .two = "sl"},   {.three = "sme", .two = "se"},
         {.three = "smo", .two = "sm"},   {.three = "sna", .two = "sn"},
         {.three = "snd", .two = "sd"},   {.three = "som", .two = "so"},
         {.three = "sot", .two = "st"},   {.three = "spa", .two = "es"},
         {.three = "sqi", .two = "sq"},   {.three = "srd", .two = "sc"},
         {.three = "srp", .two = "sr"},   {.three = "ssw", .two = "ss"},
         {.three = "sun", .two = "su"},   {.three = "swa", .two = "sw"},
         {.three = "swe", .two = "sv"},   {.three = "tah", .two = "ty"},
         {.three = "tam", .two = "ta"},   {.three = "tat", .two = "tt"},
         {.three = "tel", .two = "te"},   {.three = "tgk", .two = "tg"},
         {.three = "tgl", .two = "tl"},   {.three = "tha", .two = "th"},
         {.three = "tib", .two = "bo"},   {.three = "tir", .two = "ti"},
         {.three = "ton", .two = "to"},   {.three = "tsn", .two = "tn"},
         {.three = "tso", .two = "ts"},   {.three = "tuk", .two = "tk"},
         {.three = "tur", .two = "tr"},   {.three = "twi", .two = "tw"},
         {.three = "uig", .two = "ug"},   {.three = "ukr", .two = "uk"},
         {.three = "urd", .two = "ur"},   {.three = "uzb", .two = "uz"},
         {.three = "ven", .two = "ve"},   {.three = "vie", .two = "vi"},
         {.three = "vol", .two = "vo"},   {.three = "wel", .two = "cy"},
         {.three = "wln", .two = "wa"},   {.three = "wol", .two = "wo"},
         {.three = "xho", .two = "xh"},   {.three = "yid", .two = "yi"},
         {.three = "yor", .two = "yo"},   {.three = "zha", .two = "za"},
         {.three = "zho", .two = "zh"},   {.three = "zul", .two = "zu"},
         {.three = "﻿aar", .two = "aa"}}
    );

    static_assert(
        std::ranges::is_sorted(rows, {}, &language_subtag_row::three),
        "binary_search needs a sorted table"
    );

    auto const* const found =
        std::ranges::lower_bound(rows, subtag, {}, &language_subtag_row::three);

    if (found == rows.end() || found->three != subtag)
        return std::nullopt;

    return found->two;
}

// --- Curated licence permissions --------------------

namespace
{

struct license_permissions_row
{
    std::string_view id;
    license_permissions permissions;
};

}  // namespace

std::optional<license_permissions> find_license_permissions(std::string_view spdx_id) noexcept
{
    // Hand-curated, because the SPDX License List carries no permissions matrix
    // and no canonical machine-readable source for one exists.
    //
    // Anything outside this table is unknown.
    constexpr auto rows = std::to_array<license_permissions_row>(
        {{.id = "CC-BY-1.0",
          .permissions = {.grants_redistribution = true, .grants_derivation = true}},
         {.id = "CC-BY-2.0",
          .permissions = {.grants_redistribution = true, .grants_derivation = true}},
         {.id = "CC-BY-2.5",
          .permissions = {.grants_redistribution = true, .grants_derivation = true}},
         {.id = "CC-BY-3.0",
          .permissions = {.grants_redistribution = true, .grants_derivation = true}},
         {.id = "CC-BY-4.0",
          .permissions = {.grants_redistribution = true, .grants_derivation = true}},
         {.id = "CC-BY-NC-1.0",
          .permissions = {.grants_redistribution = true, .grants_derivation = true}},
         {.id = "CC-BY-NC-2.0",
          .permissions = {.grants_redistribution = true, .grants_derivation = true}},
         {.id = "CC-BY-NC-2.5",
          .permissions = {.grants_redistribution = true, .grants_derivation = true}},
         {.id = "CC-BY-NC-3.0",
          .permissions = {.grants_redistribution = true, .grants_derivation = true}},
         {.id = "CC-BY-NC-4.0",
          .permissions = {.grants_redistribution = true, .grants_derivation = true}},
         {.id = "CC-BY-NC-ND-1.0",
          .permissions = {.grants_redistribution = true, .grants_derivation = false}},
         {.id = "CC-BY-NC-ND-2.0",
          .permissions = {.grants_redistribution = true, .grants_derivation = false}},
         {.id = "CC-BY-NC-ND-2.5",
          .permissions = {.grants_redistribution = true, .grants_derivation = false}},
         {.id = "CC-BY-NC-ND-3.0",
          .permissions = {.grants_redistribution = true, .grants_derivation = false}},
         {.id = "CC-BY-NC-ND-4.0",
          .permissions = {.grants_redistribution = true, .grants_derivation = false}},
         {.id = "CC-BY-NC-SA-1.0",
          .permissions = {.grants_redistribution = true, .grants_derivation = true}},
         {.id = "CC-BY-NC-SA-2.0",
          .permissions = {.grants_redistribution = true, .grants_derivation = true}},
         {.id = "CC-BY-NC-SA-2.5",
          .permissions = {.grants_redistribution = true, .grants_derivation = true}},
         {.id = "CC-BY-NC-SA-3.0",
          .permissions = {.grants_redistribution = true, .grants_derivation = true}},
         {.id = "CC-BY-NC-SA-4.0",
          .permissions = {.grants_redistribution = true, .grants_derivation = true}},
         {.id = "CC-BY-ND-1.0",
          .permissions = {.grants_redistribution = true, .grants_derivation = false}},
         {.id = "CC-BY-ND-2.0",
          .permissions = {.grants_redistribution = true, .grants_derivation = false}},
         {.id = "CC-BY-ND-2.5",
          .permissions = {.grants_redistribution = true, .grants_derivation = false}},
         {.id = "CC-BY-ND-3.0",
          .permissions = {.grants_redistribution = true, .grants_derivation = false}},
         {.id = "CC-BY-ND-4.0",
          .permissions = {.grants_redistribution = true, .grants_derivation = false}},
         {.id = "CC-BY-SA-1.0",
          .permissions = {.grants_redistribution = true, .grants_derivation = true}},
         {.id = "CC-BY-SA-2.0",
          .permissions = {.grants_redistribution = true, .grants_derivation = true}},
         {.id = "CC-BY-SA-2.5",
          .permissions = {.grants_redistribution = true, .grants_derivation = true}},
         {.id = "CC-BY-SA-3.0",
          .permissions = {.grants_redistribution = true, .grants_derivation = true}},
         {.id = "CC-BY-SA-4.0",
          .permissions = {.grants_redistribution = true, .grants_derivation = true}},
         {.id = "CC-PDDC",
          .permissions = {.grants_redistribution = true, .grants_derivation = true}},
         {.id = "CC0-1.0",
          .permissions = {.grants_redistribution = true, .grants_derivation = true}},
         {.id = "Unlicense",
          .permissions = {.grants_redistribution = true, .grants_derivation = true}}}
    );

    static_assert(
        std::ranges::is_sorted(rows, {}, &license_permissions_row::id),
        "binary_search needs a sorted table"
    );

    auto const* const found =
        std::ranges::lower_bound(rows, spdx_id, {}, &license_permissions_row::id);
    if (found == rows.end() || found->id != spdx_id)
        return std::nullopt;

    return found->permissions;
}

}  // namespace arcana::data
