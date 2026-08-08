// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

// Shared by validation_test.cpp and every tests/validation/<area>_test.cpp.

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

// Every fixture deck under tests/fixtures/validation/, as a path relative to
// FIXTURES_DIR. This is the list the coverage test walks, so a fixture missing
// from it reads as coverage and is not — add the row in the same layer that
// adds the directory.
constexpr std::array<std::string_view, 1> validation_fixtures{
    "validation/ansi/ansi-outside-root-error",
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
