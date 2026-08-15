// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include "loader.hpp"

#include "../data/text.hpp"
#include "standard_cards.hpp"
#include "toml_read.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <charconv>
#include <format>
#include <ranges>
#include <system_error>
#include <utility>

namespace arcana
{

namespace detail
{

namespace
{

namespace fs = std::filesystem;

std::string capitalize(std::string_view word)
{
    std::string result{word};

    if (!result.empty())
        result[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(result[0])));

    return result;
}

std::string default_minor_arcana_name(suit s, rank r)
{
    return std::format("{} of {}", capitalize(to_string(r)), capitalize(to_string(s)));
}

// Scrub a major arcana string
// "The Star" => star
std::string fold_major_arcana_name(std::string_view name)
{
    std::string result;
    result.reserve(name.size());
    for (char const c : name)
    {
        result.push_back(
            c == ' ' ? '_' : static_cast<char>(std::tolower(static_cast<unsigned char>(c)))
        );
    }

    constexpr std::string_view article = "the_";
    if (result.starts_with(article))
        result.erase(0, article.size());

    return result;
}

// The names file wins over deck.toml's own alt_text
std::optional<std::string> coalesce_alt_text(
    std::optional<std::string> from_names, std::optional<std::string> const& declared
)
{
    auto text = std::move(from_names).value_or(declared.value_or(""));
    if (text.empty())
        return std::nullopt;

    return text;
}

// "Raster folders are named h<height>/"
bool looks_like_raster_root(std::string_view name)
{
    return name.size() > 1 && name.front() == 'h' &&
           std::ranges::all_of(
               name.substr(1), [](unsigned char c) { return std::isdigit(c) != 0; }
           );
}

// "Files should be stored in the `ansi<lines>/` directory"
bool looks_like_ansi_root(std::string_view name)
{
    return name.starts_with("ansi") && name.size() > 4 &&
           std::ranges::all_of(
               name.substr(4), [](unsigned char c) { return std::isdigit(c) != 0; }
           );
}

}  // namespace

deck_loader::deck_loader(
    fs::path deck_root, std::shared_ptr<deck_document const> document,
    std::vector<std::string> const& languages
)
    : root_{std::move(deck_root)},
      document_{std::move(document)},
      deck_table_{document_->table["deck"].as_table()},
      names_{name_catalog::load(root_, languages)}
{
    deck_.root_path = root_;
    deck_.document_ = document_;
}

deck deck_loader::build() &&
{
    // parse toml mainly
    parse_metadata();
    parse_companions();
    parse_excluded_cards();
    parse_card_backs();
    parse_aliases();
    parse_major_arcana_remap();
    parse_custom_cards();

    // build the deck from the parsed toml
    discover_image_roots();
    build_suits();
    build_standard_majors();
    build_standard_minors();
    build_custom_majors();
    build_custom_minors();

    return std::move(deck_);
}

// --- deck.toml sections ---------------------------------------------------------

void deck_loader::parse_metadata()
{
    toml::table const& t = *deck_table_;

    deck_metadata m;
    m.identifier = get_string(t["identifier"]);
    m.schema_version = get_string_or(t["schema_version"]);
    m.name = get_string_or(t["name"]);
    m.version = get_string_or(t["version"]);
    m.icon = get_string(t["icon"]);
    m.artist = get_string(t["artist"]);
    m.creator = get_string(t["creator"]);
    m.license = get_string(t["license"]);
    m.attribution = get_string(t["attribution"]);
    m.aspect_ratio = t["aspect_ratio"].value<double>().value_or(default_aspect_ratio);
    m.description = get_string(t["description"]);
    m.created_date = get_string(t["created_date"]);
    m.updated_date = get_string(t["updated_date"]);
    m.publisher = get_string(t["publisher"]);
    m.website = get_string(t["website"]);
    m.tags = get_string_array(t["tags"]);

    deck_.metadata = std::move(m);
}

void deck_loader::parse_companions()
{
    auto const* array = (*deck_table_)["companions"]["esoterica"].as_array();
    if (array == nullptr)
        return;

    for (auto const& element : *array)
    {
        auto const* t = element.as_table();
        if (t == nullptr)
            continue;

        deck_.companions.push_back(
            esoterica_companion{
                .id = get_string_or((*t)["id"]),
                .name = get_string_or((*t)["name"]),
                .uri = get_string_or((*t)["uri"])
            }
        );
    }
}

void deck_loader::parse_excluded_cards()
{
    deck_.excluded.cards = get_string_array((*deck_table_)["excluded_cards"]["cards"]);
    deck_.excluded.reason = get_string((*deck_table_)["excluded_cards"]["reason"]);
}

void deck_loader::parse_card_backs()
{
    toml::table const& document = document_->table;

    if (auto const* card_backs = document["card_backs"].as_table())
    {
        deck_.default_card_back = get_string((*card_backs)["default"]);

        if (auto const* variants = (*card_backs)["variants"].as_table())
        {
            for (auto const& [key, value] : *variants)
            {
                auto const* t = value.as_table();
                if (t == nullptr)
                    continue;

                auto image_ref = get_string_or((*t)["image"]);
                fs::path image;

                if (!image_ref.empty())
                    image = root_ / image_ref;

                deck_.card_backs.push_back(
                    card_back_design{
                        .id = std::string(key.str()),
                        .name = get_string_or((*t)["name"]),
                        .image_ref = std::move(image_ref),
                        .image = std::move(image),
                        .description = get_string((*t)["description"]),
                        .alt_text = get_string((*t)["alt_text"]),
                        .origin = {},
                        .declared = true
                    }
                );
            }
        }
    }

    // Card backs present on disk but not declared
    std::error_code ec;
    fs::path const backs_dir = root_ / "card_backs";
    if (!fs::is_directory(backs_dir, ec))
        return;

    std::vector<card_back_design> discovered;
    for (auto const& entry : fs::directory_iterator(backs_dir, ec))
    {
        if (!entry.is_regular_file())
            continue;

        auto stem = entry.path().stem().string();
        bool const already_declared = std::ranges::any_of(
            deck_.card_backs, [&stem](card_back_design const& back) { return back.id == stem; }
        );
        if (already_declared)
            continue;

        discovered.push_back(
            card_back_design{
                .id = stem,
                .name = stem,
                .image_ref = {},
                .image = entry.path(),
                .description = std::nullopt,
                .alt_text = std::nullopt,
                .origin = {},
                .declared = false
            }
        );
    }

    std::ranges::sort(discovered, {}, &card_back_design::id);
    deck_.card_backs.insert(deck_.card_backs.end(), discovered.begin(), discovered.end());
}

void deck_loader::parse_aliases()
{
    toml::table const& document = document_->table;

    suit_aliases_ = get_string_map(document["aliases"]["suits"]);
    court_aliases_ = get_string_map(document["aliases"]["courts"]);

    // v2 gives ranks no manifest field, so the resolved names are private to
    // the deck and reached through display_rank_name()
    deck_.rank_names_ = court_aliases_;
}

void deck_loader::parse_major_arcana_remap()
{
    auto const* t = document_->table["remap_major_arcana"].as_table();
    if (t == nullptr)
        return;

    for (auto const& [key, value] : *t)
    {
        auto s = value.value<std::string>();
        if (!s)
            continue;

        auto const key_text = key.str();
        int position = 0;
        auto const [ptr, ec] =
            std::from_chars(key_text.data(), key_text.data() + key_text.size(), position);

        if (ec == std::errc{} && ptr == key_text.data() + key_text.size())
            remapped_positions_.emplace(fold_major_arcana_name(*s), position);
        // else: unparseable key, skipped permissively.
        // TODO: this should emit a warning
    }
}

std::vector<custom_card_def> deck_loader::parse_minor_custom_cards(toml::array const& array)
{
    std::vector<custom_card_def> result;

    for (auto const& element : array)
    {
        auto const* t = element.as_table();
        if (t == nullptr)
            continue;

        result.push_back(
            custom_card_def{
                .id = get_string_or((*t)["id"]),
                .name = get_string_or((*t)["name"]),
                .image_ref = get_string_or((*t)["image"]),
                .alt_text = get_string((*t)["alt_text"]),
                .position = (*t)["position"].value<int>()
            }
        );
    }

    return result;
}

void deck_loader::parse_custom_cards()
{
    auto const* custom = document_->table["custom_cards"].as_table();
    if (custom == nullptr)
        return;

    if (auto const* major = (*custom)["major_arcana"].as_table())
    {
        for (auto const& [key, value] : *major)
        {
            auto const* t = value.as_table();
            if (t == nullptr)
                continue;

            custom_major_cards_.push_back(
                custom_card_def{
                    .id = get_string_or((*t)["id"], std::string(key.str())),
                    .name = get_string_or((*t)["name"]),
                    .image_ref = get_string_or((*t)["image"]),
                    .alt_text = get_string((*t)["alt_text"]),
                    .position = (*t)["position"].value<int>()
                }
            );
        }
    }

    if (auto const* minor = (*custom)["minor_arcana"].as_table())
    {
        for (auto const& [key, value] : *minor)
        {
            auto const* t = value.as_table();
            if (t == nullptr)
                continue;

            custom_suit_def suit_def;
            suit_def.key = std::string(key.str());
            suit_def.name = get_string_or((*t)["name"], suit_def.key);
            if (auto const* cards = (*t)["cards"].as_array())
                suit_def.cards = parse_minor_custom_cards(*cards);

            custom_suits_.push_back(std::move(suit_def));
        }
    }
}

// --- assets on disk -------------------------------------------------------------

void deck_loader::discover_image_roots()
{
    std::error_code ec;

    if (fs::is_directory(root_ / "scalable", ec))
        image_roots_.emplace_back("scalable");

    for (auto const& entry : fs::directory_iterator(root_, ec))
    {
        if (!entry.is_directory())
            continue;

        auto const name = entry.path().filename().string();
        if (looks_like_raster_root(name) || looks_like_ansi_root(name))
            image_roots_.push_back(name);
    }
}

card_image deck_loader::image_from_relative_path(std::string_view relative_path) const
{
    card_image result;
    result.path = root_ / relative_path;

    auto const slash = relative_path.find('/');
    result.source_dir = std::string(relative_path.substr(0, slash));

    if (looks_like_raster_root(result.source_dir))
    {
        result.kind = image_kind::raster;
        result.height = std::stoi(result.source_dir.substr(1));
    }
    else if (looks_like_ansi_root(result.source_dir))
    {
        result.kind = image_kind::ansi;
        result.lines = std::stoi(result.source_dir.substr(4));
    }
    else
    {
        // TODO: should this be the fallback?
        result.kind = image_kind::scalable;
    }

    return result;
}

std::vector<card_image> deck_loader::scan_images_for(
    fs::path const& relative_stem_dir, std::string_view stem
) const
{
    std::vector<card_image> result;
    std::error_code ec;

    for (auto const& image_root : image_roots_)
    {
        fs::path const dir = root_ / image_root / relative_stem_dir;
        if (!fs::is_directory(dir, ec))
            continue;

        for (auto const& entry : fs::directory_iterator(dir, ec))
        {
            if (!entry.is_regular_file())
                continue;

            if (entry.path().stem().string() == stem)
            {
                auto const relative =
                    fs::path(image_root) / relative_stem_dir / entry.path().filename();
                result.push_back(image_from_relative_path(relative.generic_string()));
                break;
            }
        }
    }

    return result;
}

// --- card materialization -------------------------------------------------------

bool deck_loader::is_excluded(std::string const& canonical_id) const
{
    return std::ranges::find(deck_.excluded.cards, canonical_id) != deck_.excluded.cards.end();
}

std::string deck_loader::suit_name(std::string_view key) const
{
    if (auto const it = suit_aliases_.find(std::string(key)); it != suit_aliases_.end())
        return it->second;

    return titlecase_key(key);
}

void deck_loader::build_suits()
{
    auto const canonical_ranks = []
    {
        std::vector<std::string> keys;
        keys.reserve(standard_ranks.size());
        for (auto const r : standard_ranks) keys.emplace_back(to_string(r));
        return keys;
    }();

    auto const all_excluded =
        [this](std::string_view suit_key, std::vector<std::string> const& ranks)
    {
        return std::ranges::all_of(
            ranks, [this, suit_key](std::string const& rank_key)
            { return is_excluded(std::format("minor_arcana.{}.{}", suit_key, rank_key)); }
        );
    };

    for (auto const s : standard_suits)
    {
        auto key = std::string(to_string(s));
        deck_.suits.push_back(
            suit_info{
                .key = key,
                .name = suit_name(key),
                .ranks = canonical_ranks,
                .standard = true,
                .excluded = all_excluded(key, canonical_ranks)
            }
        );
    }

    for (auto const& custom : custom_suits_)
    {
        std::vector<std::string> ranks;
        ranks.reserve(custom.cards.size());
        for (auto const& def : custom.cards) ranks.push_back(def.id);

        deck_.suits.push_back(
            suit_info{
                .key = custom.key,
                .name = custom.name,
                .ranks = std::move(ranks),
                .standard = false,
                .excluded = false
            }
        );
    }
}

void deck_loader::build_standard_majors()
{
    for (int i = 0; i <= max_major_arcana_number; ++i)
    {
        auto id = card_id::standard_major(i);
        if (is_excluded(id.to_canonical()))
            continue;

        auto const& canonical_name = default_major_arcana_names[static_cast<std::size_t>(i)];
        auto const key = std::format("{:02d}", i);

        card c;
        c.id = std::move(id);
        c.display_name = names_.lookup("major_arcana", key).value_or(std::string(canonical_name));

        // 1.0 declares no face number; [remap_major_arcana] is a position
        auto const remapped = remapped_positions_.find(fold_major_arcana_name(canonical_name));
        c.position = remapped == remapped_positions_.end() ? i : remapped->second;
        c.alt_text = names_.lookup("alt_text", key);
        c.images = scan_images_for("major_arcana", key);

        deck_.cards.push_back(std::move(c));
    }
}

void deck_loader::build_standard_minors()
{
    for (auto const s : standard_suits)
    {
        for (auto const r : standard_ranks)
        {
            auto id = card_id::standard_minor(s, r);
            if (is_excluded(id.to_canonical()))
                continue;

            card c;
            c.id = std::move(id);
            c.display_name = names_.lookup_minor("minor_arcana", to_string(s), to_string(r))
                                 .value_or(default_minor_arcana_name(s, r));
            c.display_suit = deck_.display_suit_name(s);
            c.display_rank = deck_.display_rank_name(r);
            c.alt_text = names_.lookup_minor("alt_text", to_string(s), to_string(r));
            c.images =
                scan_images_for(fs::path("minor_arcana") / std::string(to_string(s)), to_string(r));

            deck_.cards.push_back(std::move(c));
        }
    }
}

void deck_loader::build_custom_majors()
{
    for (auto const& def : custom_major_cards_)
    {
        card c;
        c.id = card_id::custom_major(def.id);
        c.display_name = names_.lookup("major_arcana", def.id).value_or(def.name);
        c.position = def.position;  // nullopt unless the deck declared a position
        c.alt_text = coalesce_alt_text(names_.lookup("alt_text", def.id), def.alt_text);
        if (!def.image_ref.empty())
            c.images.push_back(image_from_relative_path(def.image_ref));

        deck_.cards.push_back(std::move(c));
    }
}

void deck_loader::build_custom_minors()
{
    for (auto const& suit_def : custom_suits_)
    {
        for (auto const& def : suit_def.cards)
        {
            card c;
            c.id = card_id::custom_minor(suit_def.key, def.id);
            c.display_name =
                names_.lookup_minor("minor_arcana", suit_def.key, def.id).value_or(def.name);
            c.display_suit = deck_.display_suit_name(suit_def.key);
            c.display_rank = deck_.display_rank_name(def.id);
            c.alt_text = coalesce_alt_text(
                names_.lookup_minor("alt_text", suit_def.key, def.id), def.alt_text
            );
            if (!def.image_ref.empty())
                c.images.push_back(image_from_relative_path(def.image_ref));

            deck_.cards.push_back(std::move(c));
        }
    }
}

}  // namespace detail

std::expected<deck, error> load_deck(
    std::filesystem::path const& deck_directory, std::vector<std::string> const& languages
)
{
    auto document = detail::load_deck_document(deck_directory);
    if (!document)
        return std::unexpected(std::move(document.error()));

    return detail::deck_loader{deck_directory, *std::move(document), languages}.build();
}

}  // namespace arcana
