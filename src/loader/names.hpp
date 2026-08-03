// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#pragma once

#include <toml++/toml.hpp>

#include <filesystem>
#include <optional>
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

    // @return The string at [<section>].<key>
    [[nodiscard]] std::optional<std::string> lookup(
        std::string_view section, std::string_view key
    ) const;

    // @return The string at [<section>.<suit_key>].<rank_key>
    [[nodiscard]] std::optional<std::string> lookup_minor(
        std::string_view section, std::string_view suit_key, std::string_view rank_key
    ) const;

    // @return False when no names file was found or failed to parse
    [[nodiscard]] bool loaded() const noexcept
    {
        return loaded_;
    }

  private:
    toml::table table_;
    bool loaded_ = false;
};

}  // namespace arcana::detail
