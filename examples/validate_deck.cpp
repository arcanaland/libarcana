// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

// Judge a deck against the diagnostic catalogue.
//
//   ./validate_deck ~/decks rider-waite-smith

#include <arcana/library.hpp>
#include <arcana/validation.hpp>

#include <cstddef>
#include <print>
#include <string>
#include <string_view>
#include <vector>

namespace
{

std::string_view label(arcana::severity level)
{
    switch (level)
    {
        case arcana::severity::pedantic:
            return "pedantic";
        case arcana::severity::info:
            return "info";
        case arcana::severity::warning:
            return "warning";
        case arcana::severity::error:
            return "error";
    }
    return "?";
}

}  // namespace

int main(int argc, char** argv)
{
    arcana::library_options options;
    if (argc > 1)
        options.roots.emplace_back(argv[1]);

    arcana::deck_library library{options};

    if (library.decks().empty())
    {
        std::println("no decks found");
        return 1;
    }

    std::string const directory_name = argc > 2 ? argv[2] : library.decks().front().directory_name;

    auto loaded = library.load(directory_name);
    if (!loaded)
    {
        // A deck that will not load is not a deck that validates: the loader
        // rejects it before any rule runs.
        std::println("could not load {}: {}", directory_name, loaded.error().message);
        return 1;
    }

    // The catalogue is written from the specification and is always ahead of
    // the checks, so its size is not the number of things judged here.
    std::println("catalogue: {} rules", arcana::rules().size());

    std::vector<arcana::diagnostic> const findings = arcana::validate(**loaded);

    std::size_t errors = 0;
    for (arcana::diagnostic const& finding : findings)
    {
        errors += static_cast<std::size_t>(finding.level == arcana::severity::error);

        std::println("{:8} {:32} {}", label(finding.level), finding.code, finding.message);

        // Where the finding is, when the rule knows: a card, a file, or a key
        // in deck.toml.
        if (finding.card)
            std::println("         card {}", *finding.card);
        if (finding.path)
            std::println("         path {}", finding.path->string());
        if (finding.key)
            std::println("         key  {}", *finding.key);
    }

    std::println("{}: {} findings, {} of them errors", directory_name, findings.size(), errors);

    // A warning is a deck the packager should fix; only an error means
    // non-conforming. Both still load.
    return errors == 0 ? 0 : 1;
}
