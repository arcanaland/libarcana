// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#pragma once

#include <toml++/toml.hpp>

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace arcana::detail
{

// The names/<lang>.toml catalog for one deck
class name_catalog
{
  public:
    // Prefers names/<language>.toml, then names/en.toml, then any .toml in names/
    //
    // Returns an unloaded catalog when the deck has no names/ directory, no .toml
    // in it, or the chosen file fails to parse.
    [[nodiscard]] static name_catalog load(
        std::filesystem::path const& deck_root, std::optional<std::string> const& language
    );

    // The string at [<section>].<key>
    [[nodiscard]] std::optional<std::string> lookup(
        std::string_view section, std::string_view key
    ) const;

    // The string at [<section>.<suit_key>].<rank_key>
    [[nodiscard]] std::optional<std::string> lookup_minor(
        std::string_view section, std::string_view suit_key, std::string_view rank_key
    ) const;

    // False when no names file was found or it failed to parse
    [[nodiscard]] bool loaded() const noexcept
    {
        return loaded_;
    }

  private:
    toml::table table_;
    bool loaded_ = false;
};

}  // namespace arcana::detail
