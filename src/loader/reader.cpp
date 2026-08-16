// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include "reader.hpp"

#include "../data/text.hpp"
#include "deck_access.hpp"
#include "discovery.hpp"
#include "names.hpp"
#include "ordering.hpp"
#include "standard_cards.hpp"
#include "toml_read.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <format>
#include <functional>
#include <map>
#include <set>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace arcana::detail
{

namespace
{

namespace fs = std::filesystem;

constexpr std::string_view default_minor_name_template = "{rank} of {suit}";

// A vocabulary table such as [deck.origin] sorted by system
std::vector<origin_term> read_origin(toml::node_view<toml::node const> const& node)
{
    std::vector<origin_term> terms;

    for (auto const& [system, term] : get_string_map(node))
        if (!term.empty())
            terms.push_back({.system = system, .term = term});

    std::ranges::sort(terms, {}, &origin_term::system);
    return terms;
}

// What a [cards."<ref>"] entry supplies. Discovery creates the card; this only
// annotates one it already made
struct card_annotation
{
    std::optional<std::string> name;
    std::optional<std::string> alt_text;
    std::optional<std::string> number;
    std::optional<int> position;
    std::optional<std::string> image;
    std::vector<origin_term> origin;
};

// One image root's contribution
struct root_index
{
    image_root root;

    // base -> the unsuffixed file, for major_arcana/
    std::map<std::string, fs::path> majors;

    // suit key -> rank key -> the unsuffixed file
    std::map<std::string, std::map<std::string, fs::path>> minors;

    // design key -> the file
    std::map<std::string, fs::path> card_backs;
};

card_image image_at(image_root const& root, fs::path path)
{
    return {
        .source_dir = root.name,
        .path = std::move(path),
        .kind = root.kind,
        .height = root.height,
        .lines = root.lines
    };
}

// The filename base a card's artwork is stored under
//   - the two-digit key, the custom major key or the rank key
std::string base_of(card_id const& id)
{
    switch (id.cls)
    {
        case card_class::standard_major:
            return std::format("{:02}", id.number);
        case card_class::custom_major:
            return id.custom_id;
        case card_class::standard_minor:
            return std::string{to_string(id.standard_rank)};
        case card_class::custom_minor:
            return id.custom_id;
    }

    return {};
}

std::string suit_key_of(card_id const& id)
{
    if (id.cls == card_class::standard_minor)
        return std::string{to_string(id.standard_suit)};

    return id.suit_key;
}

// The two names §6.3.1 composes a minor arcanum's name from
struct minor_parts
{
    std::string_view rank;
    std::string_view suit;
};

std::string compose_minor_name(std::string_view name_template, minor_parts const& parts)
{
    constexpr std::string_view rank_placeholder = "{rank}";
    constexpr std::string_view suit_placeholder = "{suit}";

    std::string result;
    result.reserve(name_template.size());

    for (std::size_t pos = 0; pos < name_template.size();)
    {
        if (name_template.compare(pos, rank_placeholder.size(), rank_placeholder) == 0)
        {
            result += parts.rank;
            pos += rank_placeholder.size();
        }
        else if (name_template.compare(pos, suit_placeholder.size(), suit_placeholder) == 0)
        {
            result += parts.suit;
            pos += suit_placeholder.size();
        }
        else
        {
            result.push_back(name_template[pos]);
            ++pos;
        }
    }

    return result;
}

class reader
{
  public:
    reader(fs::path root, std::shared_ptr<deck_document const> document, name_catalog names)
        : root_{std::move(root)}, document_{std::move(document)}, names_{std::move(names)}
    {
    }

    deck read();

  private:
    [[nodiscard]] toml::table const& table() const noexcept
    {
        return document_->table;
    }

    void read_metadata();

    void discover();
    void discover_majors(image_root const& root, root_index& index);
    void discover_minors(image_root const& root, root_index& index);
    static void discover_backs(image_root const& root, root_index& index);

    void build_suits();

    void read_annotations();
    [[nodiscard]] std::set<std::string> wanted_cards() const;
    void attach_images(card& c) const;
    void build_cards();
    void mark_excluded_suits();

    void read_card_backs();

    void resolve_names();
    void resolve_suit_names();
    void resolve_rank_names();
    void resolve_card_names();
    void resolve_back_names();
    void name_major(card& c);
    void name_minor(card& c, std::string_view name_template);

    // The string a name file supplies at a key path, if any
    [[nodiscard]] std::optional<std::string> from_names(
        std::span<std::string_view const> path
    ) const
    {
        return names_.lookup(path);
    }

    // The manifest fallback that sits between the name file and the last resort
    [[nodiscard]] std::optional<std::string> annotated_name(std::string const& canonical) const
    {
        auto const found = annotations_.find(canonical);
        return found == annotations_.end() ? std::nullopt : found->second.name;
    }

    [[nodiscard]] std::vector<origin_term> with_deck_origin(std::vector<origin_term>&& declared) const
    {
        for (auto const& term : deck_origin_)
            if (std::ranges::find(declared, term.system, &origin_term::system) == declared.end())
                declared.push_back(term);

        std::ranges::sort(declared, {}, &origin_term::system);
        return declared;
    }

    fs::path root_;
    std::shared_ptr<deck_document const> document_;
    name_catalog names_;

    deck deck_;
    std::vector<origin_term> deck_origin_;
    std::vector<root_index> roots_;

    // Every canonical ID a file created, whether or not the file was a variant
    std::set<std::string> discovered_;

    // Suit keys created by a minor_arcana/<suit>/ directory holding a file
    std::set<std::string> discovered_suits_;

    std::map<std::string, card_annotation> annotations_;

    // Rank keys the deck uses, so that [name.rank] is read for each of them
    std::set<std::string> rank_keys_;
};

void reader::read_metadata()
{
    auto const deck_table = table()["deck"];
    auto& metadata = deck_.metadata;

    metadata.identifier = get_string(deck_table["identifier"]);
    metadata.schema_version = get_string_or(deck_table["schema_version"]);
    metadata.name = get_string_or(deck_table["name"]);
    metadata.version = get_string_or(deck_table["version"]);
    metadata.icon = get_string(deck_table["icon"]);
    metadata.creator = get_string(deck_table["creator"]);
    metadata.artist = get_string(deck_table["artist"]);
    metadata.license = get_string(deck_table["license"]);
    metadata.attribution = get_string(deck_table["attribution"]);
    metadata.description = get_string(deck_table["description"]);
    metadata.publisher = get_string(deck_table["publisher"]);
    metadata.tags = get_string_array(deck_table["tags"]);

    if (auto const ratio = deck_table["aspect_ratio"].value<double>())
        metadata.aspect_ratio = *ratio;

    deck_origin_ = read_origin(deck_table["origin"]);
    metadata.origin = deck_origin_;

    auto const excluded = table()["excluded_cards"];
    deck_.excluded.cards = get_string_array(excluded["cards"]);
    deck_.excluded.reason = get_string(excluded["reason"]);

    deck_.default_card_back = get_string(table()["card_backs"]["default"]);
}

void reader::discover_majors(image_root const& root, root_index& index)
{
    for (auto& asset :
         discover_directory(root.path / "major_arcana", root.kind, /*allow_variants=*/true))
    {
        auto const id = card_id::parse(std::format("major_arcana.{}", asset.base));
        if (!id)
            continue;

        discovered_.insert(id->to_canonical());

        if (asset.variant_key.empty())
            index.majors.emplace(asset.base, std::move(asset.path));
    }
}

void reader::discover_minors(image_root const& root, root_index& index)
{
    std::error_code ec;
    for (auto const& entry : fs::directory_iterator(root.path / "minor_arcana", ec))
    {
        if (!entry.is_directory(ec))
            continue;

        auto const suit_key = entry.path().filename().string();
        if (!suit_from_string(suit_key) && !is_custom_name(suit_key))
            continue;

        for (auto& asset : discover_directory(entry.path(), root.kind, /*allow_variants=*/true))
        {
            auto const id = card_id::parse(std::format("minor_arcana.{}.{}", suit_key, asset.base));

            // TODO: A canonical suit holding a custom rank key
            if (!id)
                continue;

            discovered_suits_.insert(suit_key);
            discovered_.insert(id->to_canonical());

            if (asset.variant_key.empty())
                index.minors[suit_key].emplace(asset.base, std::move(asset.path));
        }
    }
}

void reader::discover_backs(image_root const& root, root_index& index)
{
    for (auto& asset :
         discover_directory(root.path / "card_backs", root.kind, /*allow_variants=*/false))
        if (is_custom_name(asset.base))
            index.card_backs.emplace(asset.base, std::move(asset.path));
}

void reader::discover()
{
    for (auto const& root : find_image_roots(root_))
    {
        root_index index{.root = root};

        discover_majors(root, index);
        discover_minors(root, index);
        discover_backs(root, index);

        roots_.push_back(std::move(index));
    }
}

void reader::build_suits()
{
    auto const suits_table = table()["suits"];

    auto add = [&](std::string key, bool standard)
    {
        suit_info info{.key = std::move(key), .standard = standard};

        auto const declared = suits_table[info.key];
        info.name = get_string_or(declared["name"]);

        // `ranks` on a canonical suit replaces its canonical sequence
        info.ranks = get_string_array(declared["ranks"]);
        if (info.ranks.empty() && standard)
            for (auto const r : standard_ranks) info.ranks.emplace_back(to_string(r));

        for (auto const& rank_key : info.ranks) rank_keys_.insert(rank_key);

        deck_.suits.push_back(std::move(info));
    };

    for (auto const s : standard_suits) add(std::string{to_string(s)}, /*standard=*/true);

    for (auto const& key : discovered_suits_)
        if (!suit_from_string(key))
            add(key, /*standard=*/false);
}

void reader::read_annotations()
{
    auto const* const cards_table = table()["cards"].as_table();
    if (cards_table == nullptr)
        return;

    for (auto const& [key, value] : *cards_table)
    {
        std::string const reference{key.str()};
        if (reference.contains(':'))
            continue;

        auto const entry = toml::node_view<toml::node const>{value};

        card_annotation annotation;
        annotation.name = get_string(entry["name"]);
        annotation.alt_text = get_string(entry["alt_text"]);
        annotation.number = get_string(entry["number"]);
        annotation.image = get_string(entry["image"]);
        annotation.origin = read_origin(entry["origin"]);

        if (auto const position = entry["position"].value<std::int64_t>())
            annotation.position = static_cast<int>(*position);

        annotations_.emplace(reference, std::move(annotation));
    }
}

std::set<std::string> reader::wanted_cards() const
{
    std::set<std::string> wanted = discovered_;

    // The seventy-eight canonical slots exist for every deck
    for (int number = 0; number <= max_major_arcana_number; ++number)
        wanted.insert(std::format("major_arcana.{:02}", number));

    for (auto const s : standard_suits)
        for (auto const r : standard_ranks)
            wanted.insert(std::format("minor_arcana.{}.{}", to_string(s), to_string(r)));

    return wanted;
}

void reader::attach_images(card& c) const
{
    auto const base = base_of(c.id);

    for (auto const& index : roots_)
    {
        std::map<std::string, fs::path> const* level = nullptr;

        if (c.id.is_major())
            level = &index.majors;
        else if (auto const suit = index.minors.find(suit_key_of(c.id)); suit != index.minors.end())
            level = &suit->second;

        if (level == nullptr)
            continue;

        if (auto const found = level->find(base); found != level->end())
            c.images.push_back(image_at(index.root, found->second));
    }

    // An explicit `image` path wins outright over discovery
    auto const annotation = annotations_.find(c.canonical_id());
    if (annotation == annotations_.end())
        return;

    auto const& declared = annotation->second.image;
    if (!declared.has_value())
        return;

    c.images.clear();
    c.images.push_back({.source_dir = {}, .path = root_ / *declared, .kind = image_kind::scalable});
}

void reader::build_cards()
{
    read_annotations();

    for (auto const& canonical : wanted_cards())
    {
        if (std::ranges::find(deck_.excluded.cards, canonical) != deck_.excluded.cards.end())
            continue;

        auto const id = card_id::parse(canonical);
        if (!id)
            continue;

        card c{.id = *id};

        if (auto const annotation = annotations_.find(canonical); annotation != annotations_.end())
        {
            c.number = annotation->second.number;
            c.alt_text = annotation->second.alt_text;
            c.origin = annotation->second.origin;

            if (id->is_major())
                c.position = annotation->second.position;
        }

        c.origin = with_deck_origin(std::move(c.origin));
        attach_images(c);

        if (!id->is_major())
            rank_keys_.insert(base_of(*id));

        deck_.cards.push_back(std::move(c));
    }

    mark_excluded_suits();
}

void reader::mark_excluded_suits()
{
    for (auto& info : deck_.suits)
        info.excluded = std::ranges::none_of(
            deck_.cards,
            [&](card const& c) { return !c.id.is_major() && suit_key_of(c.id) == info.key; }
        );
}

void reader::read_card_backs()
{
    // The designs a deck has are the union of the stems found across every card
    // back directory, plus every declared key carrying an explicit `image`
    std::map<std::string, fs::path> found;

    // The top-level card_backs/ is a root of no declared kind or size, so
    // neither chain alone describes it: the raster chain is the documented
    // simple form and scalable fills in for a design shipped only as SVG. It
    // seeds the map, so any image root below overwrites what it supplies
    for (auto const kind : {image_kind::scalable, image_kind::raster})
        for (auto& asset : discover_directory(root_ / "card_backs", kind, /*allow_variants=*/false))
            if (is_custom_name(asset.base))
                found.insert_or_assign(asset.base, std::move(asset.path));

    // The model holds one image per design where the spec resolves per kind and
    // size, so a preference is unavoidable: scalable, then the largest raster,
    // then the largest ANSI. Lower wins
    auto const preference = [](image_root const& root)
    {
        switch (root.kind)
        {
            case image_kind::scalable:
                return std::pair{0, 0};
            case image_kind::raster:
                return std::pair{1, -root.height.value_or(0)};
            case image_kind::ansi:
                return std::pair{2, -root.lines.value_or(0)};
        }

        std::unreachable();
    };

    // Worst-ranked first, so the best-ranked root is the last to write and wins
    std::vector<root_index const*> ranked;
    ranked.reserve(roots_.size());
    for (auto const& index : roots_) ranked.push_back(&index);

    std::ranges::sort(
        ranked, std::greater{}, [&](root_index const* index) { return preference(index->root); }
    );

    for (auto const* const index : ranked)
        for (auto const& [key, path] : index->card_backs) found.insert_or_assign(key, path);

    auto const designs = table()["card_backs"]["designs"];

    std::set<std::string> keys;
    for (auto const& entry : found) keys.insert(entry.first);

    if (auto const* const declared = designs.as_table(); declared != nullptr)
        for (auto const& [key, value] : *declared)
            if (get_string(toml::node_view<toml::node const>{value}["image"]))
                keys.insert(std::string{key.str()});

    for (auto const& key : keys)
    {
        auto const declared = designs[key];

        card_back_design design{.id = key};
        design.name = get_string_or(declared["name"]);
        design.description = get_string(declared["description"]);
        design.alt_text = get_string(declared["alt_text"]);
        design.declared = declared.as_table() != nullptr;
        design.origin = with_deck_origin(read_origin(declared["origin"]));

        if (auto const explicit_path = get_string(declared["image"]))
        {
            design.image_ref = *explicit_path;
            design.image = root_ / *explicit_path;
        }
        else if (auto const discovered = found.find(key); discovered != found.end())
        {
            design.image = discovered->second;
            design.image_ref = fs::relative(discovered->second, root_).generic_string();
        }

        deck_.card_backs.push_back(std::move(design));
    }
}

void reader::resolve_suit_names()
{
    for (auto& info : deck_.suits)
    {
        std::array const path{
            std::string_view{"name"}, std::string_view{"suit"}, std::string_view{info.key}
        };

        if (auto const named = from_names(path))
            info.name = *named;
    }
}

void reader::resolve_rank_names()
{
    auto& rank_names = deck_access::rank_names(deck_);

    for (auto const& key : rank_keys_)
    {
        std::array const path{
            std::string_view{"name"}, std::string_view{"rank"}, std::string_view{key}
        };

        if (auto const named = from_names(path))
            rank_names.emplace(key, *named);
    }
}

void reader::name_major(card& c)
{
    auto const base = base_of(c.id);

    std::array const path{
        std::string_view{"name"}, std::string_view{"card"}, std::string_view{"major_arcana"},
        std::string_view{base}
    };
    std::array const alt_path{
        std::string_view{"alt_text"}, std::string_view{"card"}, std::string_view{"major_arcana"},
        std::string_view{base}
    };

    if (auto const named = from_names(path))
        c.display_name = *named;
    else if (auto const declared = annotated_name(c.canonical_id()))
        c.display_name = *declared;
    else if (c.id.cls == card_class::standard_major && c.id.number <= max_major_arcana_number)
        c.display_name = default_major_arcana_names[static_cast<std::size_t>(c.id.number)];
    else
        // fallback
        c.display_name = titlecase_key(base);

    if (auto const alt = from_names(alt_path))
        c.alt_text = *alt;
}

void reader::name_minor(card& c, std::string_view name_template)
{
    auto const base = base_of(c.id);
    auto const suit_key = suit_key_of(c.id);

    c.display_suit = deck_.display_suit_name(suit_key);
    c.display_rank = deck_.display_rank_name(base);

    std::array const path{
        std::string_view{"name"}, std::string_view{"card"}, std::string_view{"minor_arcana"},
        std::string_view{suit_key}, std::string_view{base}
    };
    std::array const alt_path{
        std::string_view{"alt_text"}, std::string_view{"card"}, std::string_view{"minor_arcana"},
        std::string_view{suit_key}, std::string_view{base}
    };

    if (auto const named = from_names(path))
        c.display_name = *named;
    else if (auto const declared = annotated_name(c.canonical_id()))
        c.display_name = *declared;
    else
        // A minor arcanum's name is composed
        c.display_name =
            compose_minor_name(name_template, {.rank = c.display_rank, .suit = c.display_suit});

    if (auto const alt = from_names(alt_path))
        c.alt_text = *alt;
}

void reader::resolve_card_names()
{
    std::array const template_path{
        std::string_view{"name"}, std::string_view{"card"}, std::string_view{"minor_arcana"},
        std::string_view{"name_template"}
    };
    auto const name_template =
        from_names(template_path).value_or(std::string{default_minor_name_template});

    for (auto& c : deck_.cards)
        if (c.id.is_major())
            name_major(c);
        else
            name_minor(c, name_template);
}

void reader::resolve_back_names()
{
    for (auto& design : deck_.card_backs)
    {
        std::array const path{
            std::string_view{"name"}, std::string_view{"card_back"}, std::string_view{design.id}
        };
        std::array const alt_path{
            std::string_view{"alt_text"}, std::string_view{"card_back"}, std::string_view{design.id}
        };

        if (auto const named = from_names(path))
            design.name = *named;
        else if (design.name.empty())
            design.name = titlecase_key(design.id);

        if (auto const alt = from_names(alt_path))
            design.alt_text = *alt;
    }
}

void reader::resolve_names()
{
    // Suits and ranks because a minor arcana needs them
    resolve_suit_names();
    resolve_rank_names();

    resolve_card_names();
    resolve_back_names();
}

deck reader::read()
{
    deck_.root_path = root_;

    read_metadata();
    discover();
    build_suits();
    build_cards();
    read_card_backs();
    resolve_names();

    sort_cards(deck_.cards.begin(), deck_.cards.end(), deck_.suits.cbegin(), deck_.suits.cend());

    deck_access::document(deck_) = document_;
    return std::move(deck_);
}

}  // namespace

std::expected<deck, error> read_deck(
    fs::path const& deck_directory, std::shared_ptr<deck_document const> document,
    std::vector<std::string> const& languages
)
{
    reader source{
        deck_directory, std::move(document), name_catalog::load(deck_directory, languages)
    };

    return source.read();
}

}  // namespace arcana::detail
