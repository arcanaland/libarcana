// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include <arcana/paths.hpp>

#include <catch2/catch_test_macros.hpp>

TEST_CASE("xdg_data_home resolves deterministically under a fake root", "[paths]")
{
    REQUIRE(arcana::xdg_data_home("/fake/root") == "/fake/root/.local/share");
}

TEST_CASE("xdg_config_home resolves deterministically under a fake root", "[paths]")
{
    REQUIRE(arcana::xdg_config_home("/fake/root") == "/fake/root/.config");
}

TEST_CASE("deck_library_path is XDG_DATA_HOME/tarot/decks under a fake root", "[paths]")
{
    REQUIRE(arcana::deck_library_path("/fake/root") == "/fake/root/.local/share/tarot/decks");
}

TEST_CASE("enumerate_deck_directories is empty when nothing is installed", "[paths]")
{
    REQUIRE(arcana::enumerate_deck_directories("/fake/root/with/no/decks").empty());
}
