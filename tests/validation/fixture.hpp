// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#pragma once

#include <arcana/deck.hpp>
#include <arcana/validation.hpp>

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <filesystem>
#include <string_view>
#include <vector>

namespace arcana_test
{

// Every fixture deck the corpus has. A directory missing from this list is
// invisible to the coverage test, so a layer adds its rows here.
constexpr std::array<std::string_view, 7> validation_fixtures{
    "validation/ansi/ansi-outside-root-error", "validation/ids/card-references-error",
    "validation/ids/custom-names-error",       "validation/ids/identifier-shape-error",
    "validation/ids/identifiers-error",        "validation/ids/identifiers-valid",
    "validation/ids/no-identifier-error",
};

// Load a fixture deck and run the full catalogue over it.
inline std::vector<arcana::diagnostic> validate_fixture(std::string_view relative)
{
    auto loaded = arcana::load_deck(std::filesystem::path{FIXTURES_DIR} / relative);
    REQUIRE(loaded.has_value());
    return arcana::validate(*loaded);
}

inline std::vector<std::string_view> codes_of(std::vector<arcana::diagnostic> const& found)
{
    std::vector<std::string_view> codes;
    codes.reserve(found.size());
    for (auto const& one : found) codes.push_back(one.code);

    return codes;
}

}  // namespace arcana_test
