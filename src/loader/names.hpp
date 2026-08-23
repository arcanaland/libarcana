// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#pragma once

#include <toml++/toml.hpp>

#include <filesystem>
#include <initializer_list>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace arcana::detail
{

// The names/<lang>.toml catalog for one deck
class name_catalog
{
  public:
    // Tries each language, then names/en.toml, then any .toml in names/
    //
    // @returns Empty catalog when the deck has no name files or they can't be parsed
    [[nodiscard]] static name_catalog load(
        std::filesystem::path const& deck_root, std::vector<std::string> const& languages
    );

    // The string at a key path, where each element is one whole TOML key
    //
    // Not a dotted string: a variant reference is a single key containing dots
    [[nodiscard]] std::optional<std::string> lookup(std::span<std::string_view const> path) const;

    // The same lookup, spelled at the call site: lookup({"name", "card", key})
    [[nodiscard]] std::optional<std::string> lookup(
        std::initializer_list<std::string_view> path
    ) const
    {
        return lookup(std::span{path.begin(), path.size()});
    }

    // @return False when no names file was found or failed to parse
    [[nodiscard]] bool loaded() const noexcept
    {
        return loaded_;
    }

  private:
    toml::table table_;
    bool loaded_ = false;
};

// §6.3.1: the template a deck's minor arcana names are composed from when no
// name file supplies one
inline constexpr std::string_view default_minor_name_template = "{rank} of {suit}";

// The two names §6.3.1 composes a minor arcanum's name from
struct minor_parts
{
    std::string_view rank;
    std::string_view suit;
};

// Substitutes {rank} and {suit} in `name_template`
[[nodiscard]] std::string compose_minor_name(
    std::string_view name_template, minor_parts const& parts
);

}  // namespace arcana::detail
