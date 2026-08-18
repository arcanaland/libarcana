// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include "workloads.hpp"

#include <discovery.hpp>
#include <document.hpp>
#include <summary.hpp>

#include <arcana/deck.hpp>
#include <arcana/library.hpp>
#include <arcana/validation.hpp>

#include <algorithm>
#include <array>
#include <system_error>
#include <utility>

namespace arcana::bench
{

namespace
{

namespace fs = std::filesystem;

std::array const registry{
    workload{
        .name = "document",
        .description = "parse deck.toml and nothing else",
        .run = [](context const& c)
        { return detail::load_deck_document(c.deck).has_value() ? std::size_t{1} : 0; }
    },
    workload{
        .name = "discover",
        .description = "walk the image roots for the card IDs a 2.0 deck has",
        .run = [](context const& c) { return detail::discover_card_ids(c.deck).size(); }
    },
    workload{
        .name = "summary",
        .description = "load_deck_summary, the library-scan unit",
        .run =
            [](context const& c)
        {
            auto const summary = detail::load_deck_summary(c.deck);
            return summary ? summary->card_count : 0;
        }
    },
    workload{
        .name = "load",
        .description = "load_deck, the whole model",
        .run =
            [](context const& c)
        {
            auto const d = load_deck(c.deck);
            return d ? d->cards.size() : 0;
        }
    },
    workload{
        .name = "validate",
        .description = "load_deck then validate it",
        .run =
            [](context const& c)
        {
            auto const d = load_deck(c.deck);
            if (!d)
                return std::size_t{0};

            // The cards are in the sink because a clean deck has no
            // diagnostics, and a zero sink is how the runner spots a workload
            // that has stopped doing anything
            return d->cards.size() + validate(*d).size();
        }
    },
    workload{
        .name = "refresh",
        .description = "deck_library scan: one summary per deck",
        .wants_library = true,
        .run =
            [](context const& c)
        {
            deck_library library{library_options{.roots = {c.library}}};
            return library.decks().size();
        }
    },
    workload{
        .name = "library-load",
        .description = "scan a library, then load every deck in it",
        .wants_library = true,
        .run = [](context const& c)
        {
            deck_library library{library_options{.roots = {c.library}}};

            std::size_t cards = 0;
            for (auto const& summary : library.decks())
                if (auto const d = load_deck(summary.path))
                    cards += d->cards.size();

            return cards;
        }
    }
};

}  // namespace

std::span<workload const> workloads()
{
    return registry;
}

workload const* find_workload(std::string_view name)
{
    auto const found = std::ranges::find(registry, name, &workload::name);
    return found == registry.end() ? nullptr : &*found;
}

std::vector<fs::path> decks_in(fs::path const& library)
{
    std::vector<fs::path> decks;

    std::error_code ec;
    for (auto const& entry : fs::directory_iterator(library, ec))
        if (entry.is_directory(ec) && fs::exists(entry.path() / "deck.toml", ec))
            decks.push_back(entry.path());

    std::ranges::sort(decks);
    return decks;
}

}  // namespace arcana::bench
