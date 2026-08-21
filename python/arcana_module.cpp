// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include <arcana/card.hpp>
#include <arcana/deck.hpp>
#include <arcana/error.hpp>
#include <arcana/library.hpp>
#include <arcana/paths.hpp>
#include <arcana/version.hpp>

#include <nanobind/nanobind.h>
#include <nanobind/operators.h>
#include <nanobind/stl/filesystem.h>
#include <nanobind/stl/map.h>
#include <nanobind/stl/optional.h>
#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/string_view.h>
#include <nanobind/stl/unordered_map.h>
#include <nanobind/stl/vector.h>

#include <cstddef>
#include <cstdint>
#include <expected>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace nb = nanobind;

using namespace arcana;

namespace
{

PyObject* deck_error_type = nullptr;

[[noreturn]] void raise_deck_error(error const& problem)
{
    nb::object exception =
        nb::steal(PyObject_CallFunction(deck_error_type, "s", problem.message.c_str()));

    exception.attr("code") = problem.code;
    exception.attr("message") = problem.message;

    PyErr_SetObject(deck_error_type, exception.ptr());
    throw nb::python_error();
}

template <typename T>
T unwrap(std::expected<T, error> result)
{
    if (!result)
        raise_deck_error(result.error());

    return std::move(*result);
}

template <typename T>
struct span_view
{
    std::span<T const> items;
};

using deck_summary_view = span_view<deck_summary>;

template <typename T>
void bind_span_view(nb::module_& m)
{
    nb::class_<span_view<T>>(m, "deck_summary_view")
        .def("__len__", [](span_view<T> const& self) { return self.items.size(); })
        .def(
            "__getitem__", [](span_view<T> const& self, std::size_t index) -> T const&
            { return self.items[index]; }, nb::rv_policy::reference_internal
        );
}

}  // namespace

NB_MODULE(_core, m)  // NOLINT
{
    // TODO:
    // m.doc() = "";

    m.attr("__version__") = std::string{library_version()};

    deck_error_type = PyErr_NewException("arcana_tarot.deck_error", PyExc_RuntimeError, nullptr);
    m.attr("deck_error") = nb::borrow(deck_error_type);

    // --- Shape: enum class ----------------------------------------------------

    nb::enum_<error_code>(m, "error_code")
        .value("not_found", error_code::not_found)
        .value("parse_error", error_code::parse_error)
        .value("io_error", error_code::io_error)
        .value("invalid_argument", error_code::invalid_argument);

    nb::enum_<arcana_kind>(m, "arcana_kind")
        .value("major_arcana", arcana_kind::major_arcana)
        .value("minor_arcana", arcana_kind::minor_arcana);

    nb::enum_<suit>(m, "suit")
        .value("wands", suit::wands)
        .value("cups", suit::cups)
        .value("swords", suit::swords)
        .value("pentacles", suit::pentacles);

    nb::enum_<rank>(m, "rank")
        .value("ace", rank::ace)
        .value("two", rank::two)
        .value("three", rank::three)
        .value("four", rank::four)
        .value("five", rank::five)
        .value("six", rank::six)
        .value("seven", rank::seven)
        .value("eight", rank::eight)
        .value("nine", rank::nine)
        .value("ten", rank::ten)
        .value("page", rank::page)
        .value("knight", rank::knight)
        .value("queen", rank::queen)
        .value("king", rank::king);

    nb::enum_<card_class>(m, "card_class")
        .value("standard_major", card_class::standard_major)
        .value("custom_major", card_class::custom_major)
        .value("standard_minor", card_class::standard_minor)
        .value("custom_minor", card_class::custom_minor);

    nb::enum_<image_kind>(m, "image_kind")
        .value("scalable", image_kind::scalable)
        .value("raster", image_kind::raster)
        .value("ansi", image_kind::ansi);

    // --- Shape: free functions over string_view -------------------------------

    m.def("to_string", [](suit s) { return to_string(s); });
    m.def("to_string", [](rank r) { return to_string(r); });
    m.def("suit_from_string", &suit_from_string);
    m.def("rank_from_string", &rank_from_string);
    m.def("is_valid_identifier", &is_valid_identifier);

    // --- Shape: aggregate of strings and optionals ----------------------------

    nb::class_<error>(m, "error")
        .def(nb::init<>())
        .def_rw("code", &error::code)
        .def_rw("message", &error::message);

    nb::class_<card_id>(m, "card_id")
        .def(nb::init<>())
        .def_rw("cls", &card_id::cls)
        .def_rw("number", &card_id::number)
        .def_rw("standard_suit", &card_id::standard_suit)
        .def_rw("standard_rank", &card_id::standard_rank)
        .def_rw("suit_key", &card_id::suit_key)
        .def_rw("custom_id", &card_id::custom_id)
        .def_static("standard_major", &card_id::standard_major)
        .def_static("standard_minor", &card_id::standard_minor)
        .def_static("custom_major", &card_id::custom_major)
        .def_static("custom_minor", &card_id::custom_minor)
        .def("is_major", &card_id::is_major)
        .def("kind", &card_id::kind)
        .def("is_custom", &card_id::is_custom)
        .def("to_canonical", &card_id::to_canonical)
        // Shape: std::expected on a static factory.
        .def_static(
            "parse", [](std::string_view canonical) { return unwrap(card_id::parse(canonical)); },
            nb::arg("canonical_id")
        )
        .def(nb::self == nb::self)  // NOLINT(misc-redundant-expression)
        .def(
            "__repr__", [](card_id const& self) { return "<card_id " + self.to_canonical() + ">"; }
        );

    // Shape: std::filesystem::path
    nb::class_<card_image>(m, "card_image")
        .def(nb::init<>())
        .def_rw("source_dir", &card_image::source_dir)
        .def_rw("path", &card_image::path)
        .def_rw("kind", &card_image::kind)
        .def_rw("height", &card_image::height)
        .def_rw("lines", &card_image::lines)
        .def_rw("variant_key", &card_image::variant_key);

    nb::class_<origin_term>(m, "origin_term")
        .def(nb::init<>())
        .def_rw("system", &origin_term::system)
        .def_rw("term", &origin_term::term)
        .def(nb::self == nb::self)  // NOLINT(misc-redundant-expression)
        .def(
            "__repr__", [](origin_term const& self)
            { return "<origin_term " + self.system + "=" + self.term + ">"; }
        );

    nb::class_<card_variant>(m, "card_variant")
        .def(nb::init<>())
        .def_rw("key", &card_variant::key)
        .def_rw("display_name", &card_variant::display_name)
        .def_rw("alt_text", &card_variant::alt_text)
        .def_rw("origin", &card_variant::origin)
        .def(nb::self == nb::self)  // NOLINT(misc-redundant-expression)
        .def(
            "__repr__", [](card_variant const& self) { return "<card_variant " + self.key + ">"; }
        );

    nb::class_<card>(m, "card")
        .def(nb::init<>())
        .def_rw("id", &card::id)
        .def_rw("display_name", &card::display_name)
        .def_rw("display_suit", &card::display_suit)
        .def_rw("display_rank", &card::display_rank)
        .def_rw("number", &card::number)
        .def_rw("position", &card::position)
        .def_rw("alt_text", &card::alt_text)
        .def_rw("origin", &card::origin)
        // Shape: std::vector<T> by value.
        .def_rw("images", &card::images)
        .def_rw("default_variant", &card::default_variant)
        .def_rw("variants", &card::variants)
        .def("canonical_id", &card::canonical_id)
        .def("variant_keys", &card::variant_keys)
        .def("images_for_variant", &card::images_for_variant, nb::arg("variant_key"))
        // Each accessor is bound twice, once per C++ overload: the bare form and
        // the one that takes a variant key. The static_casts pick the overload.
        .def(
            "best_raster_for_height",
            static_cast<std::optional<card_image> (card::*)(int) const>(
                &card::best_raster_for_height
            ),
            nb::arg("target_height")
        )
        .def(
            "best_raster_for_height",
            static_cast<std::optional<card_image> (card::*)(int, std::string_view) const>(
                &card::best_raster_for_height
            ),
            nb::arg("target_height"), nb::arg("variant_key")
        )
        .def(
            "best_ansi_for_lines",
            static_cast<std::optional<card_image> (card::*)(int) const>(&card::best_ansi_for_lines),
            nb::arg("target_lines")
        )
        .def(
            "best_ansi_for_lines",
            static_cast<std::optional<card_image> (card::*)(int, std::string_view) const>(
                &card::best_ansi_for_lines
            ),
            nb::arg("target_lines"), nb::arg("variant_key")
        )
        .def(
            "scalable_image",
            static_cast<std::optional<card_image> (card::*)() const>(&card::scalable_image)
        )
        .def(
            "scalable_image",
            static_cast<std::optional<card_image> (card::*)(std::string_view) const>(
                &card::scalable_image
            ),
            nb::arg("variant_key")
        );

    nb::class_<suit_info>(m, "suit_info")
        .def(nb::init<>())
        .def_rw("key", &suit_info::key)
        .def_rw("name", &suit_info::name)
        .def_rw("ranks", &suit_info::ranks)
        .def_rw("standard", &suit_info::standard)
        .def_rw("excluded", &suit_info::excluded);

    nb::class_<card_back_design>(m, "card_back_design")
        .def(nb::init<>())
        .def_rw("id", &card_back_design::id)
        .def_rw("name", &card_back_design::name)
        .def_rw("image_ref", &card_back_design::image_ref)
        .def_rw("image", &card_back_design::image)
        .def_rw("description", &card_back_design::description)
        .def_rw("alt_text", &card_back_design::alt_text)
        .def_rw("origin", &card_back_design::origin)
        .def_rw("declared", &card_back_design::declared);

    nb::class_<deck_metadata>(m, "deck_metadata")
        .def(nb::init<>())
        .def_rw("identifier", &deck_metadata::identifier)
        .def_rw("schema_version", &deck_metadata::schema_version)
        .def_rw("name", &deck_metadata::name)
        .def_rw("version", &deck_metadata::version)
        .def_rw("icon", &deck_metadata::icon)
        .def_rw("creator", &deck_metadata::creator)
        .def_rw("artist", &deck_metadata::artist)
        .def_rw("license", &deck_metadata::license)
        .def_rw("attribution", &deck_metadata::attribution)
        .def_rw("description", &deck_metadata::description)
        .def_rw("publisher", &deck_metadata::publisher)
        .def_rw("website", &deck_metadata::website)
        .def_rw("aspect_ratio", &deck_metadata::aspect_ratio)
        .def_rw("tags", &deck_metadata::tags)
        .def_rw("origin", &deck_metadata::origin)
        // Populated only for a 1.0 deck; 2.0 states neither
        .def_rw("created_date", &deck_metadata::created_date)
        .def_rw("updated_date", &deck_metadata::updated_date);

    m.def("schema_major", &schema_major, nb::arg("metadata"));

    nb::class_<deck_summary>(m, "deck_summary")
        .def(nb::init<>())
        .def_rw("directory_name", &deck_summary::directory_name)
        .def_rw("path", &deck_summary::path)
        .def_rw("identifier", &deck_summary::identifier)
        .def_rw("name", &deck_summary::name)
        .def_rw("version", &deck_summary::version)
        .def_rw("artist", &deck_summary::artist)
        .def_rw("icon", &deck_summary::icon)
        .def_rw("card_count", &deck_summary::card_count);

    nb::class_<malformed_deck>(m, "malformed_deck")
        .def(nb::init<>())
        .def_rw("directory_name", &malformed_deck::directory_name)
        .def_rw("path", &malformed_deck::path)
        .def_rw("problem", &malformed_deck::problem);

    // --- Shape: type held by an incomplete-type shared_ptr --------------------
    nb::class_<deck>(m, "deck")
        .def_ro("root_path", &deck::root_path)
        .def_ro("metadata", &deck::metadata)
        .def_ro("cards", &deck::cards)
        .def_ro("suits", &deck::suits)
        .def_prop_ro("default_card_back", &deck::default_card_back)
        .def_ro("card_backs", &deck::card_backs)
        .def("default_card_back_design", &deck::default_card_back_design)
        .def("cards_of_kind", &deck::cards_of_kind, nb::arg("kind"))
        .def("cards_in_suit", &deck::cards_in_suit, nb::arg("key"))
        .def("find_card", &deck::find_card, nb::arg("id"))
        .def("random_card", &deck::random_card, nb::arg("seed"))
        .def("exclusion_reason", &deck::exclusion_reason, nb::arg("canonical_id"))
        .def("source_toml", &deck::source_toml);

    m.def(
        "load_deck",
        [](std::filesystem::path const& deck_directory, std::vector<std::string> const& languages)
        { return unwrap(load_deck(deck_directory, languages)); }, nb::arg("deck_directory"),
        nb::arg("languages") = std::vector<std::string>{}
    );

    // --- Shape: the span view -------------------------------------------------

    bind_span_view<deck_summary>(m);

    // --- Shape: class with private state, invariants and a mutable cache ------

    nb::class_<library_options>(m, "library_options")
        .def(
            "__init__",
            [](library_options* self, std::vector<std::filesystem::path> roots,
               std::optional<std::filesystem::path> reference_deck,
               std::vector<std::string> languages)
            {
                new (self) library_options{
                    .roots = std::move(roots),
                    .reference_deck = std::move(reference_deck),
                    .languages = std::move(languages),
                };
            },
            nb::arg("roots") = std::vector<std::filesystem::path>{},
            nb::arg("reference_deck") = nb::none(),
            nb::arg("languages") = std::vector<std::string>{}
        )
        .def_rw("roots", &library_options::roots)
        .def_rw("reference_deck", &library_options::reference_deck)
        .def_rw("languages", &library_options::languages);

    nb::class_<deck_library>(m, "deck_library")
        .def(nb::init<library_options>(), nb::arg("options") = library_options{})

        // copy decks to Python inside instead of passing a span
        .def(
            "decks", [](deck_library const& self)
            { return std::vector<deck_summary>(self.decks().begin(), self.decks().end()); }
        )
        .def(
            "malformed_decks",
            [](deck_library const& self)
            {
                return std::vector<malformed_deck>(
                    self.malformed_decks().begin(), self.malformed_decks().end()
                );
            }
        )
        .def(
            "roots", [](deck_library const& self)
            { return std::vector<std::filesystem::path>(self.roots().begin(), self.roots().end()); }
        )
        .def(
            "languages", [](deck_library const& self)
            { return std::vector<std::string>(self.languages().begin(), self.languages().end()); }
        )

        .def(
            "decks_view", [](deck_library const& self) { return deck_summary_view{self.decks()}; },
            nb::rv_policy::move, nb::keep_alive<0, 1>()
        )

        .def("reference", &deck_library::reference)
        .def("reference_path", &deck_library::reference_path)
        .def("find", &deck_library::find, nb::arg("directory_name"))
        .def("find_all_by_identifier", &deck_library::find_all_by_identifier, nb::arg("identifier"))

        .def(
            "load", [](deck_library const& self, std::string_view directory_name)
            { return unwrap(self.load(directory_name)); }, nb::arg("directory_name")
        )
        .def(
            "load_external",
            [](deck_library const& self, std::filesystem::path const& deck_directory)
            { return unwrap(self.load_external(deck_directory)); }, nb::arg("deck_directory")
        )
        .def(
            "load_reference", [](deck_library const& self) { return unwrap(self.load_reference()); }
        )

        .def("refresh", &deck_library::refresh);

    // --- Paths ----------------------------------------------------------------

    m.def("deck_library_path", &paths::deck_library_path, nb::arg("home") = std::nullopt);
}
