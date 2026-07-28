// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include <arcana/version.hpp>

#include <catch2/catch_test_macros.hpp>

TEST_CASE("library_version reports the project version", "[version]")
{
    REQUIRE(arcana::library_version() == "0.1.0");
}
