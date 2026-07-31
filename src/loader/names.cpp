// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include "names.hpp"

#include "toml_read.hpp"

#include <system_error>
#include <utility>

namespace arcana::detail
{

namespace
{

namespace fs = std::filesystem;

// "pt_BR" and "pt-BR" both reduce to "pt". Empty when the tag carries no region.
std::string_view base_language(std::string_view tag)
{
    auto const separator = tag.find_first_of("_-");
    if (separator == std::string_view::npos)
        return {};

    return tag.substr(0, separator);
}

fs::path names_file_for(fs::path const& names_dir, std::string_view language)
{
    if (language.empty())
        return {};

    std::error_code ec;
    fs::path candidate = names_dir / (std::string(language) + ".toml");

    if (fs::is_regular_file(candidate, ec))
        return candidate;

    return {};
}

fs::path choose_names_file(fs::path const& names_dir, std::vector<std::string> const& languages)
{
    for (auto const& language : languages)
    {
        if (auto exact = names_file_for(names_dir, language); !exact.empty())
            return exact;

        if (auto base = names_file_for(names_dir, base_language(language)); !base.empty())
            return base;
    }

    if (auto english = names_file_for(names_dir, "en"); !english.empty())
        return english;

    std::error_code ec;
    for (auto const& entry : fs::directory_iterator(names_dir, ec))
        if (entry.is_regular_file() && entry.path().extension() == ".toml")
            return entry.path();

    return {};
}

}  // namespace

name_catalog name_catalog::load(
    fs::path const& deck_root, std::vector<std::string> const& languages
)
{
    name_catalog result;

    fs::path const names_dir = deck_root / "names";
    std::error_code ec;
    if (!fs::is_directory(names_dir, ec))
        return result;

    auto const chosen = choose_names_file(names_dir, languages);
    if (chosen.empty())
        return result;

    auto parsed = toml::parse_file(chosen.string());
    if (!parsed)
        return result;

    result.table_ = std::move(parsed).table();
    result.loaded_ = true;
    return result;
}

std::optional<std::string> name_catalog::lookup(
    std::string_view section, std::string_view key
) const
{
    if (!loaded_)
        return std::nullopt;

    return get_string(table_[section][key]);
}

std::optional<std::string> name_catalog::lookup_minor(
    std::string_view section, std::string_view suit_key, std::string_view rank_key
) const
{
    if (!loaded_)
        return std::nullopt;

    return get_string(table_[section][suit_key][rank_key]);
}

}  // namespace arcana::detail
