// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include "facts.hpp"

#include "../data/ascii.hpp"
#include "../data/identifiers.hpp"
#include "assets.hpp"
#include "context.hpp"
#include "spec.hpp"

#include <toml++/toml.hpp>

#include <algorithm>
#include <cstdint>
#include <functional>
#include <map>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace arcana::validation
{

namespace
{

std::string lowered(std::string_view s)
{
    std::string folded{s};
    for (auto& c : folded) c = data::to_lower(c);

    return folded;
}

// The path declared for each key of the card back designs table.
std::map<std::string, std::string, std::less<>> declared_back_images(
    toml::table const& doc, std::uint8_t major
)
{
    std::map<std::string, std::string, std::less<>> found;

    auto const* designs = card_back_designs_table(doc, major);
    if (designs == nullptr)
        return found;

    for (auto const& [key, value] : *designs)
    {
        auto const* one = value.as_table();
        if (one == nullptr)
            continue;

        if (auto const* image = (*one)["image"].as_string())
            found.emplace(key.str(), image->get());
    }

    return found;
}

std::vector<back_file> classify_back_files(
    std::span<deck_file const> files,
    std::map<std::string, std::string, std::less<>> const& declared
)
{
    auto const is_declared_image = [&declared](deck_file const& file)
    {
        auto const shown = file.relative.generic_string();

        return std::ranges::any_of(
            declared, [&shown](auto const& one) { return one.second == shown; }
        );
    };

    std::vector<back_file> found;
    for (auto const& file : files)
    {
        auto const where = locate_asset(file.relative);
        if (!where || !where->card_back)
            continue;

        auto const name = file.relative.filename().string();
        auto const stem = stem_of(name);

        bool const ignored =
            !is_declared_image(file) && (stem.contains('.') || !data::is_custom_name(stem) ||
                                         !chain_admits(where->kind, extension_of(name)));

        found.push_back(
            {.file = &file, .stem = std::string{stem}, .kind = where->kind, .ignored = ignored}
        );
    }

    return found;
}

// The design keys this deck has
std::vector<std::string> design_keys(
    std::vector<back_file> const& backs,
    std::map<std::string, std::string, std::less<>> const& declared
)
{
    std::vector<std::string> found;

    for (auto const& back : backs)
        if (!back.ignored)
            found.push_back(back.stem);

    for (auto const& [key, image] : declared) found.push_back(key);

    std::ranges::sort(found);
    auto const dupes = std::ranges::unique(found);
    found.erase(dupes.begin(), dupes.end());

    return found;
}

}  // namespace

deck_file const* deck_facts::file_at(std::string_view relative) const
{
    auto const found = by_path.find(relative);

    return found == by_path.end() ? nullptr : found->second;
}

bool deck_facts::reachable(deck_file const& file, root_kind wanted) const
{
    auto const where = locate_asset(file.relative);

    // The top-level card back directory has no kind and takes what it is given.
    if (where && (!where->kind || *where->kind == wanted))
        return true;

    return std::ranges::contains(declared, file.relative.generic_string());
}

deck_facts::deck_facts(std::span<deck_file const> files, toml::table const& doc, std::uint8_t major)
{
    for (auto const& file : files)
    {
        this->by_path.try_emplace(file.relative.generic_string(), &file);

        auto const directory = file.relative.parent_path().generic_string();
        auto const name = file.relative.filename().string();
        auto const stem = stem_of(name);

        this->by_stem[stem_key{directory, std::string{stem}}].push_back(&file);
        this->by_folded_stem[stem_key{directory, lowered(stem)}].push_back(&file);
    }

    this->back_images = declared_back_images(doc, major);
    this->back_files = classify_back_files(files, this->back_images);
    this->designs = design_keys(this->back_files, this->back_images);
    this->declared = declared_paths(doc);
}

}  // namespace arcana::validation
