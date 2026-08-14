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
#include <cstddef>
#include <cstdint>
#include <format>
#include <functional>
#include <map>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <utility>
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

void add_declared(
    std::vector<coined_name>& found, std::string_view name, name_site site, std::string key
)
{
    found.push_back({.name = std::string{name}, .site = site, .key = std::move(key), .path = {}});
}

// `[suits.<key>]` keys and the rank keys in their `ranks` lists.
void collect_suit_names(toml::table const& doc, std::vector<coined_name>& found)
{
    auto const* suits = doc["suits"].as_table();
    if (suits == nullptr)
        return;

    for (auto const& [key, value] : *suits)
    {
        auto const suit_key = std::string_view{key.str()};
        add_declared(found, suit_key, name_site::suit, std::format("suits.{}", suit_key));

        auto const* t = value.as_table();
        if (t == nullptr)
            continue;

        auto const* ranks = (*t)["ranks"].as_array();
        if (ranks == nullptr)
            continue;

        for (auto const& element : *ranks)
            if (auto const* rank_key = element.as_string())
                add_declared(
                    found, rank_key->get(), name_site::rank, std::format("suits.{}.ranks", suit_key)
                );
    }
}

void collect_variant_names(toml::table const& doc, std::vector<coined_name>& found)
{
    auto const* cards = doc["cards"].as_table();
    if (cards == nullptr)
        return;

    for (auto const& [card, value] : *cards)
    {
        auto const key = std::string_view{card.str()};

        if (auto const colon = key.find(':'); colon != std::string_view::npos)
            add_declared(
                found, key.substr(colon + 1), name_site::other, std::format(R"(cards."{}")", key)
            );

        auto const* t = value.as_table();
        if (t == nullptr)
            continue;

        if (auto const* fallback = (*t)["default_variant"].as_string())
            add_declared(
                found, fallback->get(), name_site::other,
                std::format(R"(cards."{}".default_variant)", key)
            );
    }
}

// Every custom name deck.toml declares
void collect_declared(toml::table const& doc, std::vector<coined_name>& found)
{
    collect_suit_names(doc, found);

    if (auto const* designs = doc["card_backs"]["designs"].as_table())
        for (auto const& [key, value] : *designs)
            add_declared(
                found, key.str(), name_site::other, std::format("card_backs.designs.{}", key.str())
            );

    collect_variant_names(doc, found);
}

// The stem up to the first .
std::string_view base_of(std::string_view filename)
{
    return filename.substr(0, filename.find('.'));
}

bool is_canonical_major_key(std::string_view base)
{
    return base.size() == 2 && base.find_first_not_of("0123456789") == std::string_view::npos;
}

bool already_collected(std::vector<coined_name> const& found, std::string_view name, name_site site)
{
    return std::ranges::any_of(
        found, [&](coined_name const& one) { return one.name == name && one.site == site; }
    );
}

// The names that can be inferred from a file's path
void collect_from_file(deck_file const& file, std::vector<coined_name>& found)
{
    std::vector<std::string> parts;
    for (auto const& component : file.relative) parts.push_back(component.string());

    if (parts.empty())
        return;

    auto const add = [&](std::string_view name, name_site site)
    {
        if (name.empty() || already_collected(found, name, site))
            return;

        found.push_back(
            {.name = std::string{name}, .site = site, .key = {}, .path = file.relative}
        );
    };

    // The last component is the file
    auto const last = parts.size() - 1;

    for (std::size_t at = 0; at < last; ++at)
    {
        // major_arcana/<base>.<ext>
        if (parts[at] == "major_arcana" && at + 1 == last)
        {
            auto const base = base_of(parts[last]);

            // A two-digit base is a canonical major arcanum
            if (!is_canonical_major_key(base))
                add(base, name_site::other);

            continue;
        }

        // minor_arcana/<suit>/<rank>.<ext>.
        if (parts[at] != "minor_arcana" || at + 1 >= last)
            continue;

        add(parts[at + 1], name_site::suit);

        if (at + 2 == last)
            add(base_of(parts[last]), name_site::rank);
    }
}

std::vector<coined_name> coined_names(std::span<deck_file const> files, toml::table const& doc)
{
    std::vector<coined_name> found;
    collect_declared(doc, found);

    // The suits, ranks and custom majors discovered from the file tree
    for (auto const& file : files) collect_from_file(file, found);

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
    this->coined = coined_names(files, doc);
}

}  // namespace arcana::validation
