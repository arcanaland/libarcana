// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include <arcana/deck.hpp>

#include <toml++/toml.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <charconv>
#include <filesystem>
#include <format>
#include <random>
#include <ranges>
#include <sstream>

namespace arcana
{

namespace detail
{

// The retained deck.toml. Defined here so toml++ never reaches include/arcana.
struct deck_document
{
    toml::table table;
};

}  // namespace detail

namespace
{

namespace fs = std::filesystem;

constexpr std::array<std::string_view, 22> default_major_arcana_names{
    "The Fool",         "The Magician", "The High Priestess", "The Empress", "The Emperor",
    "The Hierophant",   "The Lovers",   "The Chariot",        "Strength",    "The Hermit",
    "Wheel of Fortune", "Justice",      "The Hanged Man",     "Death",       "Temperance",
    "The Devil",        "The Tower",    "The Star",           "The Moon",    "The Sun",
    "Judgement",        "The World"
};

std::string capitalize(std::string_view word)
{
    std::string result{word};
    if (!result.empty())
        result[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(result[0])));
    return result;
}

// The display fallback for a spec-form key with no alias behind it: "cups" -> "Cups",
// "shooting_star" -> "Shooting Star". A key is `[a-z0-9_]+` by construction, so word
// breaks are underscores and nothing else.
std::string titlecase_key(std::string_view key)
{
    std::string result;
    result.reserve(key.size());

    bool at_word_start = true;
    for (char const c : key)
    {
        if (c == '_')
        {
            result.push_back(' ');
            at_word_start = true;
            continue;
        }
        result.push_back(
            at_word_start ? static_cast<char>(std::toupper(static_cast<unsigned char>(c))) : c
        );
        at_word_start = false;
    }

    return result;
}

std::string default_minor_arcana_name(suit s, rank r)
{
    return std::format("{} of {}", capitalize(to_string(r)), capitalize(to_string(s)));
}

// Folds a major arcana name to a comparable key: lower_case, underscores for spaces, and
// no leading "the_", so `[remap_major_arcana]` matches whether the deck writes "star",
// "The Star" or "the_star".
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

// --- toml++ accessor helpers, confined to this file -------------------------------

std::optional<std::string> get_string(toml::node_view<toml::node const> const& node)
{
    if (auto value = node.value<std::string>())
        return value;
    return std::nullopt;
}

std::string get_string_or(toml::node_view<toml::node const> const& node, std::string fallback = "")
{
    return node.value<std::string>().value_or(std::move(fallback));
}

std::vector<std::string> get_string_array(toml::node_view<toml::node const> const& node)
{
    std::vector<std::string> result;
    if (auto const* array = node.as_array())
    {
        for (auto const& element : *array)
            if (auto value = element.value<std::string>())
                result.push_back(*value);
    }
    return result;
}

// h750/h1200/h2400 are the spec's recommended resolutions, not an exhaustive list --
// "Raster folders are named h<height>/" permits any height. ansi<lines>/ is similarly
// parameterised ("32 lines recommended"), though ansi32 is the only one seen in practice
// so far. Discovering roots from the deck directory itself keeps this open-ended rather
// than hardcoding the spec's examples as if they were the whole set.
bool looks_like_raster_root(std::string_view name)
{
    return name.size() > 1 && name.front() == 'h' &&
           std::ranges::all_of(
               name.substr(1), [](unsigned char c) { return std::isdigit(c) != 0; }
           );
}

bool looks_like_ansi_root(std::string_view name)
{
    return name.starts_with("ansi") && name.size() > 4 &&
           std::ranges::all_of(
               name.substr(4), [](unsigned char c) { return std::isdigit(c) != 0; }
           );
}

// Infers the variant name ("scalable", "ansi32", "h750", ...), its family, and that
// family's size parameter from a deck-relative path such as "scalable/major_arcana/00.svg".
// The same predicates that discovered the root decide the family, so a directory can never
// be scanned as one kind and then recorded as another.
image_variant variant_from_relative_path(fs::path const& root, std::string_view relative_path)
{
    image_variant result;
    result.path = root / relative_path;

    auto const slash = relative_path.find('/');
    result.variant_name = std::string(relative_path.substr(0, slash));

    if (looks_like_raster_root(result.variant_name))
    {
        result.kind = image_kind::raster;
        result.height = std::stoi(result.variant_name.substr(1));
    }
    else if (looks_like_ansi_root(result.variant_name))
    {
        result.kind = image_kind::ansi;
        result.lines = std::stoi(result.variant_name.substr(4));
    }
    else
    {
        // discover_variant_roots only ever offers the three families, so this is `scalable`.
        result.kind = image_kind::scalable;
    }

    return result;
}

std::vector<std::string> discover_variant_roots(fs::path const& deck_root)
{
    std::vector<std::string> result;
    std::error_code ec;

    if (fs::is_directory(deck_root / "scalable", ec))
        result.emplace_back("scalable");

    for (auto const& entry : fs::directory_iterator(deck_root, ec))
    {
        if (!entry.is_directory())
            continue;
        auto const name = entry.path().filename().string();
        if (looks_like_raster_root(name) || looks_like_ansi_root(name))
            result.push_back(name);
    }

    return result;
}

std::vector<image_variant> scan_variants_for(
    fs::path const& deck_root, std::vector<std::string> const& variant_roots,
    fs::path const& relative_stem_dir, std::string_view stem
)
{
    std::vector<image_variant> result;
    std::error_code ec;

    for (auto const& variant_root : variant_roots)
    {
        fs::path const dir = deck_root / variant_root / relative_stem_dir;
        if (!fs::is_directory(dir, ec))
            continue;

        for (auto const& entry : fs::directory_iterator(dir, ec))
        {
            if (!entry.is_regular_file())
                continue;
            if (entry.path().stem().string() == stem)
            {
                auto const relative =
                    fs::path(variant_root) / relative_stem_dir / entry.path().filename();
                result.push_back(variant_from_relative_path(deck_root, relative.generic_string()));
                break;
            }
        }
    }

    return result;
}

// --- deck.toml section parsers ----------------------------------------------------

deck_metadata parse_metadata(toml::table const& deck_table)
{
    deck_metadata m;
    m.id = get_string_or(deck_table["id"]);
    m.schema_version = get_string_or(deck_table["schema_version"]);
    m.name = get_string_or(deck_table["name"]);
    m.version = get_string_or(deck_table["version"]);
    m.icon = get_string(deck_table["icon"]);
    m.author = get_string(deck_table["author"]);
    m.license = get_string(deck_table["license"]);
    m.attribution = get_string(deck_table["attribution"]);
    m.aspect_ratio = deck_table["aspect_ratio"].value<double>().value_or(default_aspect_ratio);
    m.description = get_string(deck_table["description"]);
    m.created_date = get_string(deck_table["created_date"]);
    m.updated_date = get_string(deck_table["updated_date"]);
    m.publisher = get_string(deck_table["publisher"]);
    m.website = get_string(deck_table["website"]);
    m.tags = get_string_array(deck_table["tags"]);
    return m;
}

std::vector<esoterica_companion> parse_companions(toml::table const& deck_table)
{
    std::vector<esoterica_companion> result;
    if (auto const* array = deck_table["companions"]["esoterica"].as_array())
    {
        for (auto const& element : *array)
        {
            auto const* t = element.as_table();
            if (t == nullptr)
                continue;
            result.push_back(
                esoterica_companion{
                    .id = get_string_or((*t)["id"]),
                    .name = get_string_or((*t)["name"]),
                    .uri = get_string_or((*t)["uri"])
                }
            );
        }
    }
    return result;
}

excluded_cards parse_excluded_cards(toml::table const& deck_table)
{
    excluded_cards result;
    result.cards = get_string_array(deck_table["excluded_cards"]["cards"]);
    result.reason = get_string(deck_table["excluded_cards"]["reason"]);
    return result;
}

std::vector<card_back_variant> parse_card_backs(
    fs::path const& deck_root, toml::table const& root, std::optional<std::string>& default_back
)
{
    std::vector<card_back_variant> result;

    if (auto const* card_backs = root["card_backs"].as_table())
    {
        default_back = get_string((*card_backs)["default"]);

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
                    image = deck_root / image_ref;
                result.push_back(
                    card_back_variant{
                        .id = std::string(key.str()),
                        .name = get_string_or((*t)["name"]),
                        .image_ref = std::move(image_ref),
                        .image = std::move(image),
                        .description = get_string((*t)["description"]),
                        .alt_text = get_string((*t)["alt_text"]),
                        .declared = true
                    }
                );
            }
        }
    }

    // Card backs present on disk but not declared. Deck spec v1.0 describes card_backs/ as
    // a plain directory of images, and decks in the wild ship backs they never name in
    // deck.toml, so a consumer that only reads [card_backs.variants] misses them.
    std::error_code ec;
    fs::path const backs_dir = deck_root / "card_backs";
    if (fs::is_directory(backs_dir, ec))
    {
        std::vector<card_back_variant> discovered;
        for (auto const& entry : fs::directory_iterator(backs_dir, ec))
        {
            if (!entry.is_regular_file())
                continue;

            auto stem = entry.path().stem().string();
            bool const already_declared = std::ranges::any_of(
                result, [&stem](card_back_variant const& back) { return back.id == stem; }
            );
            if (already_declared)
                continue;

            discovered.push_back(
                card_back_variant{
                    .id = stem,
                    .name = stem,
                    .image_ref = {},
                    .image = entry.path(),
                    .description = std::nullopt,
                    .alt_text = std::nullopt,
                    .declared = false
                }
            );
        }

        // directory_iterator has no ordering guarantee; sorting keeps a load reproducible.
        std::ranges::sort(discovered, {}, &card_back_variant::id);
        result.insert(result.end(), discovered.begin(), discovered.end());
    }

    return result;
}

std::unordered_map<std::string, std::string> parse_string_map(
    toml::node_view<toml::node const> const& node
)
{
    std::unordered_map<std::string, std::string> result;

    if (auto const* t = node.as_table())
    {
        for (auto const& [key, value] : *t)
            if (auto s = value.value<std::string>())
                result.emplace(key.str(), *s);
    }

    return result;
}

std::map<int, std::string> parse_major_arcana_remap(toml::table const& root)
{
    std::map<int, std::string> result;
    if (auto const* t = root["remap_major_arcana"].as_table())
    {
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
                result.emplace(position, *s);
            // else: unparseable key, skipped permissively.
        }
    }
    return result;
}

// A deck-relative image reference resolved against the deck root, with the raw text kept.
// Empty in, empty out: an absent [custom_cards] image must stay distinguishable from one
// that resolves to the deck root itself.
void set_image(custom_card_def& def, fs::path const& deck_root, std::string image_ref)
{
    if (!image_ref.empty())
        def.image = deck_root / image_ref;
    def.image_ref = std::move(image_ref);
}

std::vector<custom_card_def> parse_minor_custom_cards(
    fs::path const& deck_root, toml::array const& array
)
{
    std::vector<custom_card_def> result;
    for (auto const& element : array)
    {
        auto const* t = element.as_table();
        if (t == nullptr)
            continue;
        custom_card_def def{
            .id = get_string_or((*t)["id"]),
            .name = get_string_or((*t)["name"]),
            .image_ref = {},
            .image = {},
            .alt_text = get_string((*t)["alt_text"]),
            .position = (*t)["position"].value<int>()
        };
        set_image(def, deck_root, get_string_or((*t)["image"]));
        result.push_back(std::move(def));
    }
    return result;
}

void parse_custom_cards(
    fs::path const& deck_root, toml::table const& root, std::vector<custom_card_def>& majors,
    std::vector<custom_suit_def>& suits
)
{
    auto const* custom = root["custom_cards"].as_table();
    if (custom == nullptr)
        return;

    if (auto const* major = (*custom)["major_arcana"].as_table())
    {
        for (auto const& [key, value] : *major)
        {
            auto const* t = value.as_table();
            if (t == nullptr)
                continue;
            custom_card_def def{
                .id = get_string_or((*t)["id"], std::string(key.str())),
                .name = get_string_or((*t)["name"]),
                .image_ref = {},
                .image = {},
                .alt_text = get_string((*t)["alt_text"]),
                .position = (*t)["position"].value<int>()
            };
            set_image(def, deck_root, get_string_or((*t)["image"]));
            majors.push_back(std::move(def));
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
                suit_def.cards = parse_minor_custom_cards(deck_root, *cards);
            suits.push_back(std::move(suit_def));
        }
    }
}

std::vector<deck_variant> parse_variants(toml::table const& root)
{
    std::vector<deck_variant> result;
    auto const* variants = root["variants"].as_table();
    if (variants == nullptr)
        return result;

    for (auto const& [key, value] : *variants)
    {
        auto const* t = value.as_table();
        if (t == nullptr)
            continue;
        result.push_back(
            deck_variant{
                .key = std::string(key.str()),
                .id = get_string_or((*t)["id"]),
                .name = get_string_or((*t)["name"]),
                .card_back = get_string((*t)["card_back"]),
                .publisher = get_string((*t)["publisher"]),
                .created_date = get_string((*t)["created_date"])
            }
        );
    }

    return result;
}

// --- names/<lang>.toml --------------------------------------------------------------

struct names_file
{
    toml::table table;
    bool loaded = false;
};

names_file load_names_file(fs::path const& deck_root, std::optional<std::string> const& language)
{
    names_file result;

    fs::path const names_dir = deck_root / "names";
    std::error_code ec;
    if (!fs::is_directory(names_dir, ec))
        return result;

    fs::path chosen;
    if (language && fs::is_regular_file(names_dir / (*language + ".toml")))
    {
        chosen = names_dir / (*language + ".toml");
    }
    else if (fs::is_regular_file(names_dir / "en.toml"))
    {
        chosen = names_dir / "en.toml";
    }
    else
    {
        for (auto const& entry : fs::directory_iterator(names_dir, ec))
        {
            if (entry.is_regular_file() && entry.path().extension() == ".toml")
            {
                chosen = entry.path();
                break;
            }
        }
    }

    if (chosen.empty())
        return result;

    auto parsed = toml::parse_file(chosen.string());
    if (!parsed)
        return result;

    result.table = std::move(parsed).table();
    result.loaded = true;
    return result;
}

std::optional<std::string> lookup_name(
    names_file const& names, std::string_view section, std::string_view key
)
{
    if (!names.loaded)
        return std::nullopt;
    return get_string(names.table[section][key]);
}

std::optional<std::string> lookup_minor_name(
    names_file const& names, std::string_view section, std::string_view suit_key,
    std::string_view rank_key
)
{
    if (!names.loaded)
        return std::nullopt;
    return get_string(names.table[section][suit_key][rank_key]);
}

}  // namespace

std::expected<deck, error> load_deck(
    fs::path const& deck_directory, std::optional<std::string> const& language
)
{
    fs::path const& root = deck_directory;
    fs::path const toml_path = root / "deck.toml";

    auto parsed = toml::parse_file(toml_path.string());
    if (!parsed)
    {
        return std::unexpected(
            error{
                .code = error_code::parse_error,
                .message = std::format(
                    "failed to parse {}: {}", toml_path.string(), parsed.error().description()
                )
            }
        );
    }

    // Retained rather than dropped on return: keys and sections no parser below names --
    // a field from a future spec version, say -- survive the load, so a writer built on
    // this parser does not silently delete what it did not understand.
    auto retained = std::make_shared<detail::deck_document>(std::move(parsed).table());
    toml::table const& document = retained->table;

    auto const* deck_table = document["deck"].as_table();
    if (deck_table == nullptr)
    {
        return std::unexpected(
            error{
                .code = error_code::parse_error,
                .message = std::format("{} has no [deck] table", toml_path.string())
            }
        );
    }

    deck result;
    result.document_ = std::move(retained);
    result.root_path = root;
    result.metadata = parse_metadata(*deck_table);
    result.companions = parse_companions(*deck_table);
    result.excluded = parse_excluded_cards(*deck_table);
    result.card_backs = parse_card_backs(root, document, result.default_card_back);
    result.suit_aliases = parse_string_map(document["aliases"]["suits"]);
    result.court_aliases = parse_string_map(document["aliases"]["courts"]);
    result.major_arcana_remap = parse_major_arcana_remap(document);
    parse_custom_cards(root, document, result.custom_major_cards, result.custom_suits);
    result.variants = parse_variants(document);

    auto const names = load_names_file(root, language);
    auto const variant_roots = discover_variant_roots(root);

    auto const is_excluded = [&result](std::string const& canonical_id)
    {
        return std::ranges::find(result.excluded.cards, canonical_id) !=
               result.excluded.cards.end();
    };

    // `[remap_major_arcana]` reads position -> which card sits there, so inverting it gives
    // the display position of each remapped card. Its keys never change a card's canonical
    // id: `major_arcana.08` stays Strength's id even in a deck that shows Justice at 8.
    std::unordered_map<std::string, int> remapped_positions;
    for (auto const& [position, name] : result.major_arcana_remap)
        remapped_positions.emplace(fold_major_arcana_name(name), position);

    // The 78 standard cards.
    for (int i = 0; i <= max_major_arcana_number; ++i)
    {
        auto id = card_id::standard_major(i);
        if (is_excluded(id.to_canonical()))
            continue;

        auto const& canonical_name = default_major_arcana_names[static_cast<std::size_t>(i)];

        card c;
        c.id = std::move(id);
        c.display_name = lookup_name(names, "major_arcana", std::format("{:02d}", i))
                             .value_or(std::string(canonical_name));

        // majors carry a number; display_suit and display_rank stay empty
        auto const remapped = remapped_positions.find(fold_major_arcana_name(canonical_name));
        c.number = remapped == remapped_positions.end() ? i : remapped->second;
        c.alt_text = lookup_name(names, "alt_text", std::format("{:02d}", i));
        c.images = scan_variants_for(root, variant_roots, "major_arcana", std::format("{:02d}", i));
        result.cards.push_back(std::move(c));
    }

    constexpr std::array<suit, 4> suits{suit::wands, suit::cups, suit::swords, suit::pentacles};
    constexpr std::array<rank, 14> ranks{rank::ace,   rank::two, rank::three, rank::four,
                                         rank::five,  rank::six, rank::seven, rank::eight,
                                         rank::nine,  rank::ten, rank::page,  rank::knight,
                                         rank::queen, rank::king};

    for (auto const s : suits)
    {
        for (auto const r : ranks)
        {
            auto id = card_id::standard_minor(s, r);
            if (is_excluded(id.to_canonical()))
                continue;

            card c;
            c.id = std::move(id);
            c.display_name = lookup_minor_name(names, "minor_arcana", to_string(s), to_string(r))
                                 .value_or(default_minor_arcana_name(s, r));
            c.display_suit = result.display_suit_name(s);
            c.display_rank = result.display_rank_name(r);
            c.alt_text = lookup_minor_name(names, "alt_text", to_string(s), to_string(r));
            c.images = scan_variants_for(
                root, variant_roots, fs::path("minor_arcana") / std::string(to_string(s)),
                to_string(r)
            );
            result.cards.push_back(std::move(c));
        }
    }

    // Custom major arcana cards.
    for (auto const& def : result.custom_major_cards)
    {
        card c;
        c.id = card_id::custom_major(def.id);
        c.display_name = lookup_name(names, "major_arcana", def.id).value_or(def.name);
        c.number = def.position;  // nullopt unless the deck declared a position
        c.alt_text = lookup_name(names, "alt_text", def.id).value_or(def.alt_text.value_or(""));
        if (c.alt_text->empty())
            c.alt_text = std::nullopt;
        if (!def.image_ref.empty())
            c.images.push_back(variant_from_relative_path(root, def.image_ref));
        result.cards.push_back(std::move(c));
    }

    // Custom suits (entire new minor arcana groups).
    for (auto const& suit_def : result.custom_suits)
    {
        for (auto const& def : suit_def.cards)
        {
            card c;
            c.id = card_id::custom_minor(suit_def.key, def.id);
            c.display_name =
                lookup_minor_name(names, "minor_arcana", suit_def.key, def.id).value_or(def.name);
            c.display_suit = result.display_suit_name(suit_def.key);
            c.display_rank = result.display_rank_name(def.id);
            c.alt_text = lookup_minor_name(names, "alt_text", suit_def.key, def.id)
                             .value_or(def.alt_text.value_or(""));
            if (c.alt_text->empty())
                c.alt_text = std::nullopt;
            if (!def.image_ref.empty())
                c.images.push_back(variant_from_relative_path(root, def.image_ref));
            result.cards.push_back(std::move(c));
        }
    }

    return result;
}

std::string deck::display_suit_name(suit s) const
{
    auto canonical = std::string(to_string(s));
    if (auto const it = suit_aliases.find(canonical); it != suit_aliases.end())
        return it->second;
    return titlecase_key(canonical);
}

std::string deck::display_suit_name(std::string_view custom_suit_key) const
{
    if (auto const it = suit_aliases.find(std::string(custom_suit_key)); it != suit_aliases.end())
        return it->second;
    for (auto const& suit_def : custom_suits)
        if (suit_def.key == custom_suit_key)
            return suit_def.name;
    return titlecase_key(custom_suit_key);
}

std::string deck::display_rank_name(rank r) const
{
    return display_rank_name(to_string(r));
}

std::string deck::display_rank_name(std::string_view custom_rank_key) const
{
    if (auto const it = court_aliases.find(std::string(custom_rank_key)); it != court_aliases.end())
        return it->second;
    return titlecase_key(custom_rank_key);
}

std::optional<std::string> deck::exclusion_reason(std::string_view canonical_id) const
{
    if (std::ranges::find(excluded.cards, canonical_id) == excluded.cards.end())
        return std::nullopt;

    return excluded.reason.value_or(std::string{});
}

std::optional<card> deck::find_card(card_id const& id) const
{
    auto const it = std::ranges::find(cards, id, &card::id);
    if (it == cards.end())
        return std::nullopt;

    return *it;
}

std::vector<suit_info> deck::suits() const
{
    auto const suit_of = [](card const& c) -> std::string_view
    {
        switch (c.id.cls)
        {
            case card_class::standard_minor:
                return to_string(c.id.standard_suit);
            case card_class::custom_minor:
                return c.id.suit_key;
            case card_class::standard_major:
            case card_class::custom_major:
                break;
        }
        return {};
    };

    auto const has_any_card = [this, &suit_of](std::string_view key)
    {
        return std::ranges::any_of(
            cards, [key, &suit_of](card const& c) { return suit_of(c) == key; }
        );
    };

    std::vector<suit_info> result;

    constexpr std::array<suit, 4> canonical{suit::wands, suit::cups, suit::swords, suit::pentacles};
    for (auto const s : canonical)
    {
        auto key = std::string(to_string(s));
        result.push_back(
            suit_info{
                .key = key,
                .display_name = display_suit_name(s),
                .standard = true,
                .excluded = !has_any_card(key)
            }
        );
    }

    // deck.toml order, not sorted: a deck author's ordering of their own suits is the
    // only ordering information there is.
    for (auto const& custom : custom_suits)
    {
        result.push_back(
            suit_info{
                .key = custom.key,
                .display_name = display_suit_name(custom.key),
                .standard = false,
                .excluded = !has_any_card(custom.key)
            }
        );
    }

    return result;
}

std::vector<card> deck::cards_of_kind(arcana_kind kind) const
{
    return cards | std::views::filter([kind](card const& c) { return c.id.kind() == kind; }) |
           std::ranges::to<std::vector>();
}

std::vector<card> deck::cards_in_suit(std::string_view key) const
{
    std::vector<card> result;

    auto const canonical = suit_from_string(key);
    for (auto const& c : cards)
    {
        bool const match =
            canonical ? c.id.cls == card_class::standard_minor && c.id.standard_suit == *canonical
                      : c.id.cls == card_class::custom_minor && c.id.suit_key == key;
        if (match)
            result.push_back(c);
    }

    return result;
}

std::optional<card> deck::random_card(std::uint64_t seed) const
{
    if (cards.empty())
        return std::nullopt;

    std::mt19937_64 engine{seed};
    std::uniform_int_distribution<std::size_t> pick{0, cards.size() - 1};
    return cards[pick(engine)];
}

std::optional<card_back_variant> deck::default_card_back_variant() const
{
    if (default_card_back)
    {
        auto const it = std::ranges::find(card_backs, *default_card_back, &card_back_variant::id);
        if (it != card_backs.end())
            return *it;
    }

    if (card_backs.size() == 1)
        return card_backs.front();

    return std::nullopt;
}

std::string deck::source_toml() const
{
    if (!document_)
        return {};

    std::ostringstream out;
    out << document_->table;
    return std::move(out).str();
}

}  // namespace arcana
