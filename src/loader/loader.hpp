// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#pragma once

#include "document.hpp"
#include "names.hpp"

#include <arcana/card.hpp>
#include <arcana/deck.hpp>

#include <toml++/toml.hpp>

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace arcana::detail
{

// One [custom_cards.major_arcana.<key>] or [custom_cards.minor_arcana.<suit>]
// card, as 1.0 spells it. A loader-internal shape: the model has one card list
// and knows nothing about "custom"
struct custom_card_def
{
    std::string id;
    std::string name;

    // The deck-relative reference exactly as written in deck.toml
    std::string image_ref;

    std::optional<std::string> alt_text;
    std::optional<int> position;
};

// An entire new suit, as 1.0's [custom_cards.minor_arcana] spells it
struct custom_suit_def
{
    std::string key;
    std::string name;
    std::vector<custom_card_def> cards;
};

// Builds a deck from a parsed deck.toml
class deck_loader
{
  public:
    deck_loader(
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

    void discover_image_roots();

    [[nodiscard]] card_image image_from_relative_path(std::string_view relative_path) const;

    // Every image root holding a file named <stem> under <relative_stem_dir>
    [[nodiscard]] std::vector<card_image> scan_images_for(
        std::filesystem::path const& relative_stem_dir, std::string_view stem
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
    std::vector<std::string> image_roots_;

    // 1.0's [aliases] tables, resolved into suit_info::name and deck::rank_names_
    std::unordered_map<std::string, std::string> suit_aliases_;
    std::unordered_map<std::string, std::string> court_aliases_;

    // 1.0's [custom_cards], before it is folded into the one card list
    std::vector<custom_card_def> custom_major_cards_;
    std::vector<custom_suit_def> custom_suits_;

    // Folded major arcana name -> the position the deck shows it at
    std::unordered_map<std::string, int> remapped_positions_;

    deck deck_;
};

}  // namespace arcana::detail
