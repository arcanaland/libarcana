// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include "reader.hpp"

#include <deck_state.hpp>
#include <discovery.hpp>
#include <names.hpp>
#include <ordering.hpp>
#include <standard_cards.hpp>
#include <text.hpp>
#include <toml_read.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <format>
#include <functional>
#include <iterator>
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

template <typename E, std::size_t N>
using vocabulary = std::array<std::pair<std::string_view, E>, N>;

// The enumerator a token names, or fallback for a token it does not know
template <typename E, std::size_t N>
E from_token(vocabulary<E, N> const& known, std::string_view token, E fallback)
{
    auto const* const found =
        std::ranges::find(known, token, &std::pair<std::string_view, E>::first);

    return found == known.end() ? fallback : found->second;
}

// The registry for supplied names.
name_source to_name_source(std::string_view token)
{
    static constexpr vocabulary<name_source, 4> known{{
        {"booklet", name_source::booklet},
        {"packaging", name_source::packaging},
        {"publisher", name_source::publisher},
        {"packager", name_source::packager},
    }};

    return from_token(known, token, name_source::other);
}

pip_style read_pips(toml::node_view<toml::node const> const& node)
{
    static constexpr vocabulary<pip_style, 2> known{{
        {"scenic", pip_style::scenic},
        {"emblematic", pip_style::emblematic},
    }};

    return from_token(known, get_string_or(node), pip_style::unstated);
}

sharing_terms read_sharing_terms(toml::node_view<toml::node const> const& node)
{
    static constexpr vocabulary<sharing_terms, 3> known{{
        {"full", sharing_terms::full},
        {"surrogate", sharing_terms::surrogate},
        {"none", sharing_terms::none},
    }};

    return from_token(known, get_string_or(node), sharing_terms::unstated);
}

// Two numbers, or nothing
std::optional<card_size> read_card_size(toml::node_view<toml::node const> const& node)
{
    auto const* const array = node.as_array();
    if (array == nullptr || array->size() != 2)
        return std::nullopt;

    auto const width = (*array)[0].value<double>();
    auto const height = (*array)[1].value<double>();
    if (!width || !height)
        return std::nullopt;

    return card_size{.width = *width, .height = *height};
}

// Every table entry in order, whatever it lacks
std::vector<deck_link> read_links(toml::node_view<toml::node const> const& node)
{
    static constexpr vocabulary<link_rel, 5> known{{
        {"homepage", link_rel::homepage},
        {"buy", link_rel::buy},
        {"artist", link_rel::artist},
        {"publisher", link_rel::publisher},
        {"source", link_rel::source},
    }};

    std::vector<deck_link> links;

    auto const* const array = node.as_array();
    if (array == nullptr)
        return links;

    for (auto const& element : *array)
    {
        auto const* const entry = element.as_table();
        if (entry == nullptr)
            continue;

        auto rel_name = get_string_or((*entry)["rel"]);
        auto const rel = from_token(known, rel_name, link_rel::other);

        links.push_back({
            .rel = rel,
            .rel_name = std::move(rel_name),
            .url = get_string_or((*entry)["url"]),
            .title = get_string((*entry)["title"]),
        });
    }

    return links;
}

// Values as written, sorted by scheme
std::vector<product_id> read_product_ids(toml::node_view<toml::node const> const& node)
{
    static constexpr vocabulary<product_scheme, 3> known{{
        {"isbn", product_scheme::isbn},
        {"gtin", product_scheme::gtin},
        {"publisher_sku", product_scheme::publisher_sku},
    }};

    std::vector<product_id> ids;

    for (auto const& [scheme, value] : get_string_map(node))
        if (!value.empty())
            ids.push_back({
                .scheme = from_token(known, scheme, product_scheme::other),
                .scheme_name = scheme,
                .value = value,
            });

    std::ranges::sort(ids, {}, &product_id::scheme_name);
    return ids;
}

std::optional<string_terms> read_string_terms(toml::node_view<toml::node const> const& node)
{
    if (!node.is_table())
        return std::nullopt;

    return string_terms{
        .source = get_string(node["source"]),
        .origin = read_origin(node["origin"]),
        .license = get_string(node["license"]),
        .license_files = get_string_array(node["license_files"]),
        .copyright = get_string(node["copyright"]),
        .rights_status = get_string(node["rights_status"]),
        .attribution = get_string(node["attribution"]),
    };
}

std::optional<supplied_name> read_supplied_name(toml::node_view<toml::node const> const& node)
{
    if (!node.is_table())
        return std::nullopt;

    auto text = get_string(node["text"]);
    if (!text)
        return std::nullopt;

    supplied_name supplied{.text = *std::move(text), .lang = get_string(node["lang"])};

    if (auto token = get_string(node["source"]))
    {
        supplied.source = to_name_source(*token);
        supplied.source_name = *std::move(token);
    }

    return supplied;
}

std::vector<std::string> read_inscription(toml::node_view<toml::node const> const& node)
{
    if (auto single = get_string(node))
        return {*std::move(single)};

    return get_string_array(node);
}

// The fields an artwork's [cards] entry shares with a variant's
void read_artwork(toml::node_view<toml::node const> const& entry, artwork_declaration& declared)
{
    declared.name = get_string(entry["name"]);
    declared.alt_text = get_string(entry["alt_text"]);
    declared.number = get_string(entry["number"]);
    declared.unnumbered = entry["unnumbered"].value_exact<bool>().value_or(false);
    declared.unnamed = entry["unnamed"].value_exact<bool>().value_or(false);
    declared.supplied = read_supplied_name(entry["supplied_name"]);
    declared.inscription = read_inscription(entry["inscription"]);
    declared.artwork_language = get_string_array(entry["artwork_language"]);
    declared.image = get_string(entry["image"]);
}

// an artwork that declares no term for a system takes the term of the artwork it sits under
std::vector<origin_term> inherit_origin(
    std::vector<origin_term>&& declared, std::vector<origin_term> const& inherited
)
{
    for (auto const& term : inherited)
        if (std::ranges::find(declared, term.system, &origin_term::system) == declared.end())
            declared.push_back(term);

    std::ranges::sort(declared, {}, &origin_term::system);
    return declared;
}

// Where a minor arcana name template was found
enum class template_layer : std::uint8_t
{
    name_file,
    manifest,
    built_in,
};

struct minor_template
{
    std::string text;
    template_layer layer = template_layer::built_in;
};

// What a [cards."<ref>"] entry supplies.
struct card_annotation
{
    card_declaration declared;
    std::vector<origin_term> origin;
};

// What a [cards."<ref>:<key>"] entry supplies.
struct variant_annotation
{
    variant_declaration declared;
    std::vector<origin_term> origin;
};

// The files one directory supplies for a card: variant key -> file, where the
// empty key is the unsuffixed file
using variant_files = std::map<std::string, fs::path>;

// A card's artwork grouped the same way, across every image root
using artwork_by_variant = std::map<std::string, std::vector<card_image>>;

// One image root's contribution
struct root_index
{
    image_root root;

    // base -> its files, for major_arcana/
    std::map<std::string, variant_files> majors;

    // suit key -> rank key -> its files
    std::map<std::string, std::map<std::string, variant_files>> minors;

    // design key -> the file
    std::map<std::string, fs::path> card_backs;
};

class reader
{
  public:
    reader(fs::path root, std::shared_ptr<deck_document const> document, name_catalog names)
        : root_{std::move(root)}, document_{std::move(document)}, names_{std::move(names)}
    {
    }

    deck_state read();

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
    void build_ranks();

    void read_annotations();
    [[nodiscard]] std::set<std::string> wanted_cards() const;
    [[nodiscard]] artwork_by_variant discovered_images(card const& c) const;
    void attach_images(card& c) const;
    void resolve_default_variant(card& c) const;
    void build_cards();
    void mark_excluded_suits();

    void read_card_backs();

    void resolve_names();

    [[nodiscard]] variant_annotation const* annotation_for(
        card const& c, std::string const& key
    ) const;
    [[nodiscard]] card_variant make_variant(card const& c, std::string const& key) const;
    void build_variants();
    void resolve_suit_names();
    void resolve_rank_names();
    void resolve_card_names();
    void resolve_back_names();
    void name_major(card& c);
    [[nodiscard]] minor_template template_for(std::string const& suit_key) const;
    [[nodiscard]] std::pair<std::string, std::string> layer_names(
        card const& c, template_layer layer
    ) const;
    [[nodiscard]] std::string composed_name(card const& c) const;
    void name_minor(card& c);

    // The string a name file supplies at a key path, if any
    [[nodiscard]] std::optional<std::string> from_names(
        std::span<std::string_view const> path
    ) const
    {
        return names_.lookup(path);
    }

    [[nodiscard]] std::optional<std::string> from_names(
        std::initializer_list<std::string_view> path
    ) const
    {
        return names_.lookup(path);
    }

    [[nodiscard]] std::vector<origin_term> with_deck_origin(
        std::vector<origin_term>&& declared
    ) const
    {
        return inherit_origin(std::move(declared), deck_origin_);
    }

    fs::path root_;
    std::shared_ptr<deck_document const> document_;
    name_catalog names_;

    deck_state deck_;
    std::vector<origin_term> deck_origin_;
    std::vector<root_index> roots_;

    // Every canonical ID a file created, whether or not the file was a variant
    std::set<std::string> discovered_;

    // Suit keys created by a minor_arcana/<suit>/ directory holding a file
    std::set<std::string> discovered_suits_;

    std::map<std::string, card_annotation> annotations_;

    // canonical ID -> variant key -> what its [cards."<ref>:<key>"] entry
    // supplies. Such an entry creates the variant where no file does, but only
    // by carrying an `image`
    std::map<std::string, std::map<std::string, variant_annotation>> variant_annotations_;

    // Rank keys the deck uses, so that deck::ranks() lists each of them
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

    metadata.pips = read_pips(deck_table["pips"]);
    metadata.default_language = get_string(deck_table["default_language"]);
    metadata.metadata_language = get_string(deck_table["metadata_language"]);
    metadata.artwork_language = get_string_array(deck_table["artwork_language"]);
    metadata.card_size_mm = read_card_size(deck_table["card_size_mm"]);
    metadata.packager = get_string(deck_table["packager"]);
    metadata.license_files = get_string_array(deck_table["license_files"]);
    metadata.copyright = get_string(deck_table["copyright"]);
    metadata.rights_status = get_string(deck_table["rights_status"]);
    metadata.redistribution = read_sharing_terms(deck_table["redistribution"]);
    metadata.derivation = read_sharing_terms(deck_table["derivation"]);
    metadata.published_date = get_string(deck_table["published_date"]);
    metadata.product_ids = read_product_ids(deck_table["product_ids"]);
    metadata.links = read_links(deck_table["links"]);

    deck_.strings = read_string_terms(table()["strings"]);

    auto const excluded = table()["excluded_cards"];
    deck_.excluded.cards = get_string_array(excluded["cards"]);
    deck_.excluded.reason = get_string(excluded["reason"]);

    deck_.default_card_back = get_string(table()["card_backs"]["default"]);

    deck_.minor_arcana_name_template = get_string(table()["minor_arcana"]["name_template"]);
}

void reader::discover_majors(image_root const& root, root_index& index)
{
    for (auto& asset :
         discover_directory(root.path / "major_arcana", root.kind, /*allow_variants=*/true))
    {
        auto const id = major_asset_id(asset.base);
        if (!id)
            continue;

        discovered_.insert(id->to_canonical());

        index.majors[asset.base].emplace(asset.variant_key, std::move(asset.path));
    }
}

void reader::discover_minors(image_root const& root, root_index& index)
{
    std::error_code ec;
    for (auto const& entry : fs::directory_iterator(root.path / "minor_arcana", ec))
    {
        if (!entry.is_directory(ec))
            continue;

        auto const suit_dir = entry.path().filename().string();
        if (!is_suit_directory(suit_dir))
            continue;

        for (auto& asset : discover_directory(entry.path(), root.kind, /*allow_variants=*/true))
        {
            auto const id = minor_asset_id(suit_dir, asset.base);
            if (!id)
                continue;

            discovered_suits_.insert(suit_dir);
            discovered_.insert(id->to_canonical());

            index.minors[suit_dir][asset.base].emplace(asset.variant_key, std::move(asset.path));
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
        info.declared.name = get_string(declared["name"]);
        info.declared.name_template = get_string(declared["name_template"]);

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

void reader::build_ranks()
{
    auto const ranks_table = table()["ranks"];

    auto const keys = std::vector<std::string>(rank_keys_.begin(), rank_keys_.end());
    deck_.ranks = deck_.ordered_ranks(keys);

    for (auto& info : deck_.ranks) info.declared.name = get_string(ranks_table[info.key]["name"]);
}

void reader::read_annotations()
{
    auto const* const cards_table = table()["cards"].as_table();
    if (cards_table == nullptr)
        return;

    for (auto const& [key, value] : *cards_table)
    {
        std::string const reference{key.str()};
        auto const entry = toml::node_view<toml::node const>{value};

        if (auto const colon = reference.find(':'); colon != std::string::npos)
        {
            variant_annotation annotation;
            read_artwork(entry, annotation.declared);
            annotation.origin = read_origin(entry["origin"]);

            variant_annotations_[reference.substr(0, colon)].insert_or_assign(
                reference.substr(colon + 1), std::move(annotation)
            );

            continue;
        }

        card_annotation annotation;
        auto& declared = annotation.declared;
        read_artwork(entry, declared);
        declared.default_variant = get_string(entry["default_variant"]);
        annotation.origin = read_origin(entry["origin"]);

        if (auto const position = entry["position"].value<std::int64_t>())
            declared.position = static_cast<int>(*position);

        annotations_.emplace(reference, std::move(annotation));
    }
}

std::set<std::string> reader::wanted_cards() const
{
    std::set<std::string> wanted = discovered_;

    // The seventy-eight canonical slots exist for every deck
    for (int number = 0; number <= max_canonical_major_arcana_number; ++number)
        wanted.insert(std::format("major_arcana.{:02}", number));

    for (auto const s : standard_suits)
        for (auto const r : standard_ranks)
            wanted.insert(std::format("minor_arcana.{}.{}", to_string(s), to_string(r)));

    return wanted;
}

// variant key -> its artwork across the roots, the empty key being the
// unsuffixed file
artwork_by_variant reader::discovered_images(card const& c) const
{
    // the filename base is the major key for a major and the rank key for a minor
    auto const base = c.id.is_major() ? major_key(c.id) : rank_key(c.id);

    artwork_by_variant by_variant;

    for (auto const& index : roots_)
    {
        std::map<std::string, variant_files> const* level = nullptr;

        if (c.id.is_major())
            level = &index.majors;
        else if (auto const suit = index.minors.find(suit_key(c.id)); suit != index.minors.end())
            level = &suit->second;

        if (level == nullptr)
            continue;

        auto const found = level->find(base);
        if (found == level->end())
            continue;

        for (auto const& [variant_key, path] : found->second)
        {
            auto image = image_at(index.root, path);
            if (!variant_key.empty())
                image.variant_key = variant_key;

            by_variant[variant_key].push_back(std::move(image));
        }
    }

    return by_variant;
}

void reader::attach_images(card& c) const
{
    auto by_variant = discovered_images(c);
    auto const canonical = c.canonical_id();

    auto const declare = [&](std::string const& variant_key, std::string const& relative)
    {
        card_image image{.source_dir = {}, .path = root_ / relative, .kind = image_kind::scalable};
        if (!variant_key.empty())
            image.variant_key = variant_key;

        by_variant[variant_key] = {std::move(image)};
    };

    if (auto const annotation = annotations_.find(canonical); annotation != annotations_.end())
        if (auto const& declared = annotation->second.declared.image)
            declare({}, *declared);

    if (auto const declared = variant_annotations_.find(canonical);
        declared != variant_annotations_.end())
        for (auto const& [variant_key, annotation] : declared->second)
            if (annotation.declared.image)
                declare(variant_key, *annotation.declared.image);

    // The unsuffixed artwork first, then the variants by key
    for (auto& entry : by_variant) std::ranges::move(entry.second, std::back_inserter(c.images));
}

void reader::resolve_default_variant(card& c) const
{
    if (auto const annotation = annotations_.find(c.canonical_id());
        annotation != annotations_.end() && annotation->second.declared.default_variant)
    {
        c.default_variant = annotation->second.declared.default_variant;
        return;
    }

    // Where the deck declares none, the unsuffixed file is the default
    if (std::ranges::any_of(c.images, [](card_image const& image) { return !image.variant_key; }))
        return;

    // A card with variant files and no unsuffixed file MUST declare
    // `default_variant`, so let's pull in the first variant key
    if (auto const keys = c.variant_keys(); !keys.empty())
        c.default_variant = keys.front();
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
            c.declared = annotation->second.declared;
            c.number = c.declared.number;
            c.alt_text = c.declared.alt_text;
            c.origin = annotation->second.origin;

            if (id->is_major())
                c.position = c.declared.position;
        }

        c.origin = with_deck_origin(std::move(c.origin));
        attach_images(c);
        resolve_default_variant(c);

        if (!id->is_major())
            rank_keys_.insert(rank_key(*id));

        deck_.cards.push_back(std::move(c));
    }

    mark_excluded_suits();
}

void reader::mark_excluded_suits()
{
    for (auto& info : deck_.suits)
        info.excluded = std::ranges::none_of(
            deck_.cards,
            [&](card const& c) { return !c.id.is_major() && suit_key(c.id) == info.key; }
        );
}

void reader::read_card_backs()
{
    std::map<std::string, fs::path> found;

    // top-level card_backs/
    for (auto const kind : {image_kind::scalable, image_kind::raster})
        for (auto& asset : discover_directory(root_ / "card_backs", kind, /*allow_variants=*/false))
            if (is_custom_name(asset.base))
                found.insert_or_assign(asset.base, std::move(asset.path));

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
            case image_kind::surrogate:
                return std::pair{3, 0};
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
        auto const entry = designs[key];

        card_back_design design{.id = key};
        design.description = get_string(entry["description"]);
        design.declared = entry.as_table() != nullptr;
        design.origin = with_deck_origin(read_origin(entry["origin"]));

        auto& declaration = design.declaration;
        declaration.name = get_string(entry["name"]);
        declaration.alt_text = get_string(entry["alt_text"]);
        declaration.inscription = read_inscription(entry["inscription"]);
        declaration.artwork_language = get_string_array(entry["artwork_language"]);
        declaration.reversible = entry["reversible"].value_exact<bool>();
        declaration.image = get_string(entry["image"]);

        if (declaration.image)
        {
            design.image_ref = *declaration.image;
            design.image = root_ / *declaration.image;
        }
        else if (auto const discovered = found.find(key); discovered != found.end())
        {
            design.image = discovered->second;
            design.image_ref = fs::relative(discovered->second, root_).generic_string();
        }

        deck_.card_backs.push_back(std::move(design));
    }
}

// The manifest step of an artwork's name chain
std::optional<std::string> manifest_name(artwork_declaration const& declared)
{
    if (declared.name)
        return declared.name;

    if (declared.supplied)
        return declared.supplied->text;

    return std::nullopt;
}

// What a [cards."<ref>:<key>"] entry supplied for one variant, if anything
variant_annotation const* reader::annotation_for(card const& c, std::string const& key) const
{
    auto const entries = variant_annotations_.find(c.canonical_id());
    if (entries == variant_annotations_.end())
        return nullptr;

    auto const found = entries->second.find(key);
    return found == entries->second.end() ? nullptr : &found->second;
}

// Ceate a variant of a card and resolve its strings / origin
card_variant reader::make_variant(card const& c, std::string const& key) const
{
    auto const* const annotation = annotation_for(c, key);

    auto const reference = std::format("{}:{}", c.canonical_id(), key);

    card_variant variant{.key = key};
    if (annotation != nullptr)
        variant.declared = annotation->declared;

    auto const& declared = variant.declared;

    auto named =
        from_names({"name", "variant", reference}).or_else([&] { return manifest_name(declared); });

    // An unnamed variant doesn't inherit its card's name
    if (!named && !declared.unnamed)
        named = c.display_name;

    variant.display_name = std::move(named).value_or(std::string{});

    if (auto const alt = from_names({"alt_text", "variant", reference}))
        variant.alt_text = *alt;
    else if (declared.alt_text)
        variant.alt_text = declared.alt_text;
    else
        variant.alt_text = c.alt_text;

    // a variant that declares no term for a system takes its card's term
    auto own = annotation == nullptr ? std::vector<origin_term>{} : annotation->origin;
    variant.origin = inherit_origin(std::move(own), c.origin);

    return variant;
}

void reader::build_variants()
{
    for (auto& c : deck_.cards)
        // variant_keys() is sorted, so `variants` comes out sorted by key
        for (auto const& key : c.variant_keys()) c.variants.push_back(make_variant(c, key));
}

// The last step of the suit and rank chains
std::string fallback_name(std::string_view key)
{
    return titlecase_key(key);
}

// The name file, then the manifest, then the fallback
void reader::resolve_suit_names()
{
    for (auto& info : deck_.suits)
        info.name = from_names({"name", "suit", info.key})
                        .or_else([&] { return info.declared.name; })
                        .value_or(fallback_name(info.key));
}

// The name file, then the manifest, then the fallback
void reader::resolve_rank_names()
{
    for (auto& info : deck_.ranks)
    {
        info.name = from_names({"name", "rank", info.key})
                        .or_else([&] { return info.declared.name; })
                        .value_or(fallback_name(info.key));

        deck_.rank_names.emplace(info.key, info.name);
    }
}

// The steps after the manifest for a major arcanum
std::optional<std::string> major_fallback(card_id const& id)
{
    if (id.cls == card_class::custom_major)
        return titlecase_key(major_key(id));

    if (id.number <= max_canonical_major_arcana_number)
        return std::string{default_major_arcana_names[static_cast<std::size_t>(id.number)]};

    // An extended major has no name here: its title-cased key would be bare
    // digits, so it is presented by its number (DECK.md#73-display-name-resolution)
    return std::nullopt;
}

void reader::name_major(card& c)
{
    auto const base = major_key(c.id);

    auto named = from_names({"name", "card", "major_arcana", base})
                     .or_else([&] { return manifest_name(c.declared); });

    // An unnamed face ends the chain at the manifest, so no name is invented
    if (!named && !c.declared.unnamed)
        named = major_fallback(c.id);

    c.display_name = std::move(named).value_or(std::string{});

    if (auto const alt = from_names({"alt_text", "card", "major_arcana", base}))
        c.alt_text = *alt;
}

// Resolve the correct name template for minor arcana
minor_template reader::template_for(std::string const& suit_key) const
{
    if (auto found = from_names({"name", "card", "minor_arcana", suit_key, "name_template"}))
        return {.text = *std::move(found), .layer = template_layer::name_file};

    if (auto found = from_names({"name", "card", "minor_arcana", "name_template"}))
        return {.text = *std::move(found), .layer = template_layer::name_file};

    if (auto const info = std::ranges::find(deck_.suits, suit_key, &suit_info::key);
        info != deck_.suits.end())
        if (auto const& declared = info->declared.name_template)
            return {.text = *declared, .layer = template_layer::manifest};

    if (auto const& declared = deck_.minor_arcana_name_template)
        return {.text = *declared, .layer = template_layer::manifest};

    return {.text = std::string{default_minor_name_template}, .layer = template_layer::built_in};
}

// {rank} or {suit} when it doesn't exist in the same layer as the template
//
// Provisional: rc.1 says a template composes with names from its own layer and
// is silent on a name that layer does not give. This takes the fallback.
std::string name_outside_layer(std::string_view key)
{
    return fallback_name(key);
}

// The rank/suit names for a template from a specific layer
std::pair<std::string, std::string> reader::layer_names(card const& c, template_layer layer) const
{
    auto const base = rank_key(c.id);
    auto const card_suit_key = suit_key(c.id);

    switch (layer)
    {
        case template_layer::name_file:
            return {
                from_names({"name", "rank", base}).value_or(name_outside_layer(base)),
                from_names({"name", "suit", card_suit_key})
                    .value_or(name_outside_layer(card_suit_key)),
            };

        case template_layer::manifest:
        {
            auto const rank = std::ranges::find(deck_.ranks, base, &rank_info::key);
            auto const suit = std::ranges::find(deck_.suits, card_suit_key, &suit_info::key);

            auto rank_name = rank == deck_.ranks.end() ? std::nullopt : rank->declared.name;
            auto suit_name = suit == deck_.suits.end() ? std::nullopt : suit->declared.name;

            return {
                std::move(rank_name).value_or(name_outside_layer(base)),
                std::move(suit_name).value_or(name_outside_layer(card_suit_key)),
            };
        }

        case template_layer::built_in:
            // The default composes with whichever layer supplied the names
            return {c.display_rank, c.display_suit};
    }

    std::unreachable();
}

// Name composed from a single layer
std::string reader::composed_name(card const& c) const
{
    auto const name_template = template_for(suit_key(c.id));
    auto const [rank_name, suit_name] = layer_names(c, name_template.layer);

    return compose_minor_name(name_template.text, {.rank = rank_name, .suit = suit_name});
}

void reader::name_minor(card& c)
{
    auto const base = rank_key(c.id);
    auto const card_suit_key = suit_key(c.id);

    c.display_suit = deck_.display_suit_name(card_suit_key);
    c.display_rank = deck_.display_rank_name(base);

    auto named = from_names({"name", "card", "minor_arcana", card_suit_key, base})
                     .or_else([&] { return manifest_name(c.declared); });

    // A minor arcanum's name is composed, unless its face shows none
    if (!named && !c.declared.unnamed)
        named = composed_name(c);

    c.display_name = std::move(named).value_or(std::string{});

    if (auto const alt = from_names({"alt_text", "card", "minor_arcana", card_suit_key, base}))
        c.alt_text = *alt;
}

void reader::resolve_card_names()
{
    for (auto& c : deck_.cards)
        if (c.id.is_major())
            name_major(c);
        else
            name_minor(c);
}

void reader::resolve_back_names()
{
    for (auto& design : deck_.card_backs)
    {
        auto const& declared = design.declaration;

        // A declared empty name falls through to the key
        if (auto const named = from_names({"name", "card_back", design.id}))
            design.name = *named;
        else if (declared.name && !declared.name->empty())
            design.name = *declared.name;
        else
            design.name = titlecase_key(design.id);

        design.alt_text = from_names({"alt_text", "card_back", design.id})
                              .or_else([&] { return declared.alt_text; });
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

deck_state reader::read()
{
    deck_.root_path = root_;

    read_metadata();
    discover();
    build_suits();
    build_cards();
    build_ranks();
    read_card_backs();
    resolve_names();
    build_variants();

    sort_cards(deck_.cards, deck_.suits);

    deck_.document = document_;
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

    return deck_builder::make(source.read());
}

}  // namespace arcana::detail
