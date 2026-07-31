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

// Builds a deck from a parsed deck.toml
class deck_loader
{
  public:
    // document must be non-null, and its table must contain a [deck] table
    deck_loader(
        std::filesystem::path deck_root, std::shared_ptr<deck_document const> document,
        std::optional<std::string> const& language
    );

    // Runs every phase and moves out the finished deck
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
    void parse_variants();

    [[nodiscard]] std::vector<custom_card_def> parse_minor_custom_cards(
        toml::array const& array
    ) const;

    // Resolves a deck-relative image reference against the deck root, keeping the
    // reference exactly as it was written
    void set_image(custom_card_def& def, std::string image_ref) const;

    // --- assets on disk -------------------------------------------------------

    void discover_image_roots();

    [[nodiscard]] card_image image_from_relative_path(std::string_view relative_path) const;

    // Every image root holding a file named <stem> under <relative_stem_dir>
    [[nodiscard]] std::vector<card_image> scan_images_for(
        std::filesystem::path const& relative_stem_dir, std::string_view stem
    ) const;

    // --- card materialization -------------------------------------------------

    void build_standard_majors();
    void build_standard_minors();
    void build_custom_majors();
    void build_custom_minors();

    [[nodiscard]] bool is_excluded(std::string const& canonical_id) const;

    std::filesystem::path root_;
    std::shared_ptr<deck_document const> document_;

    // The [deck] table, guaranteed non-null by the load_deck() precondition
    toml::table const* deck_table_ = nullptr;

    name_catalog names_;
    std::vector<std::string> image_roots_;

    // Folded major arcana name -> the position the deck shows it at
    std::unordered_map<std::string, int> remapped_positions_;

    deck deck_;
};

}  // namespace arcana::detail
