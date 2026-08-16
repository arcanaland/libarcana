// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#pragma once

#include "../discovery.hpp"
#include "../document.hpp"
#include "../names.hpp"
#include "tables.hpp"

#include <arcana/card.hpp>
#include <arcana/deck.hpp>

#include <toml++/toml.hpp>

#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace arcana::detail::v1_compat
{

// Builds a deck from a parsed 1.0 deck.toml
class deck_reader
{
  public:
    deck_reader(
        std::filesystem::path deck_root, std::shared_ptr<deck_document const> document,
        std::vector<std::string> const& languages
    );

    // Runs every parse func and moves out the finished deck
    [[nodiscard]] deck build() &&;

  private:
    // --- deck.toml sections ---------------------------------------------------

    void parse_metadata();
    void parse_companions();
    void parse_excluded_cards();
    void parse_card_backs();
    void parse_aliases();
    void parse_major_arcana_remap();
    void parse_custom_cards();

    [[nodiscard]] static std::vector<custom_card_def> parse_minor_custom_cards(
        toml::array const& array
    );

    // --- assets on disk -------------------------------------------------------

    // The card directories 1.0 draws artwork from, relative to an image root
    [[nodiscard]] std::vector<std::string> card_directories() const;

    void discover_assets();

    // A path a deck.toml spells out, which 1.0 allows anywhere under the deck
    [[nodiscard]] card_image image_from_relative_path(std::string_view relative_path) const;

    // Every image root holding a file based <base> under <directory>
    [[nodiscard]] std::vector<card_image> images_for(
        std::string const& directory, std::string_view base
    ) const;

    // --- card materialization -------------------------------------------------

    // A suit's display name from 1.0's [aliases.suits], else its title-cased key
    [[nodiscard]] std::string suit_name(std::string_view key) const;

    void build_suits();
    void build_standard_majors();
    void build_standard_minors();
    void build_custom_majors();
    void build_custom_minors();

    [[nodiscard]] bool is_excluded(std::string const& canonical_id) const;

    std::filesystem::path root_;
    std::shared_ptr<deck_document const> document_;

    // The [deck] table
    toml::table const* deck_table_ = nullptr;

    name_catalog names_;

    // One image root's card files: directory relative to the root -> base -> path
    struct root_assets
    {
        image_root root;
        std::map<std::string, std::map<std::string, std::filesystem::path>> by_directory;
    };

    std::vector<root_assets> roots_;

    // 1.0's [aliases] tables, resolved into suit_info::name and the deck's
    // private rank names
    std::unordered_map<std::string, std::string> suit_aliases_;
    std::unordered_map<std::string, std::string> court_aliases_;

    // 1.0's [custom_cards], before it is folded into the one card list
    std::vector<custom_card_def> custom_major_cards_;
    std::vector<custom_suit_def> custom_suits_;

    // Folded major arcana name -> the position the deck shows it at
    std::unordered_map<std::string, int> remapped_positions_;

    deck deck_;
};

// Read a 1.0 document into the normalized deck model
[[nodiscard]] deck read_deck(
    std::filesystem::path deck_root, std::shared_ptr<deck_document const> document,
    std::vector<std::string> const& languages
);

}  // namespace arcana::detail::v1_compat
