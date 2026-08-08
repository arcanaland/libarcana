// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include <spdx_expression.hpp>
#include <spdx_licenses.hpp>

#include <catch2/catch_test_macros.hpp>

#include <string_view>

using arcana::data::check_spdx_expression;
using arcana::data::is_spdx_exception_id;
using arcana::data::is_spdx_license_id;

TEST_CASE("the vendored list carries the identifiers a deck uses", "[spdx]")
{
    CHECK(is_spdx_license_id("MIT"));
    CHECK(is_spdx_license_id("CC0-1.0"));
    CHECK(is_spdx_license_id("CC-BY-4.0"));
    CHECK(is_spdx_license_id("CC-BY-NC-SA-3.0"));
    CHECK(is_spdx_license_id("CC-PDDC"));
    CHECK(is_spdx_license_id("Apache-2.0"));
    CHECK(is_spdx_license_id("0BSD"));

    // Deprecated identifiers are still on the list
    CHECK(is_spdx_license_id("GPL-2.0"));
}

TEST_CASE("membership is case-sensitive", "[spdx]")
{
    CHECK_FALSE(is_spdx_license_id("mit"));
    CHECK_FALSE(is_spdx_license_id("Mit"));
    CHECK_FALSE(is_spdx_license_id("cc-by-4.0"));
    CHECK_FALSE(is_spdx_license_id("CC-BY-4.O"));
}

TEST_CASE("identifiers off the list are absent", "[spdx]")
{
    CHECK_FALSE(is_spdx_license_id(""));
    CHECK_FALSE(is_spdx_license_id("NotALicense"));
    CHECK_FALSE(is_spdx_license_id("CC-BY-9.9"));
    CHECK_FALSE(is_spdx_license_id("LicenseRef-MyCustomLicense"));
}

TEST_CASE("exception identifiers are their own list", "[spdx]")
{
    CHECK(is_spdx_exception_id("Classpath-exception-2.0"));
    CHECK(is_spdx_exception_id("LLVM-exception"));

    CHECK_FALSE(is_spdx_exception_id("MIT"));
    CHECK_FALSE(is_spdx_exception_id(""));
    CHECK_FALSE(is_spdx_exception_id("Not-an-exception"));
}

TEST_CASE("well-formed expressions parse and their identifiers resolve", "[spdx]")
{
    for (std::string_view const expression : {
             "MIT",
             "CC0-1.0",
             "GPL-2.0+",
             "CC0-1.0 AND LicenseRef-PublicDomain",
             "MIT OR Apache-2.0",
             "(MIT OR Apache-2.0) AND CC-BY-4.0",
             "GPL-2.0-only WITH Classpath-exception-2.0",
             "MIT AND (CC-BY-4.0 OR CC-BY-SA-4.0)",
             "LicenseRef-MyCustomLicense",
             "DocumentRef-spdx-tool:LicenseRef-Custom",
             "  MIT   AND   CC0-1.0  ",
         })
    {
        CAPTURE(expression);
        auto const result = check_spdx_expression(expression);
        CHECK(result.well_formed);
        CHECK(result.unknown_identifier.empty());
    }
}

TEST_CASE("ill-formed expressions are reported", "[spdx]")
{
    for (std::string_view const expression : {
             "",
             "   ",
             "MIT AND",
             "AND MIT",
             "MIT OR OR Apache-2.0",
             "(MIT",
             "MIT)",
             "()",
             "MIT WITH",
             "MIT WITH AND",
             "MIT Apache-2.0",
             "MIT AND CC BY 4.0",
             "LicenseRef-Custom+",
             "DocumentRef-spdx-tool:Custom",
             "MIT and Apache-2.0",
         })
    {
        CAPTURE(expression);
        auto const result = check_spdx_expression(expression);
        CHECK_FALSE(result.well_formed);
        CHECK(result.unknown_identifier.empty());
    }
}

TEST_CASE("a well-formed expression can still name an identifier off the list", "[spdx]")
{
    auto const unknown = check_spdx_expression("NotALicense");
    CHECK(unknown.well_formed);
    CHECK(unknown.unknown_identifier == "NotALicense");

    // The first one, so that the message names something a reader can act on.
    auto const two = check_spdx_expression("MIT AND Nope-1.0 OR Also-Nope");
    CHECK(two.well_formed);
    CHECK(two.unknown_identifier == "Nope-1.0");

    auto const lowered = check_spdx_expression("mit");
    CHECK(lowered.well_formed);
    CHECK(lowered.unknown_identifier == "mit");

    auto const exception = check_spdx_expression("MIT WITH Not-An-Exception");
    CHECK(exception.well_formed);
    CHECK(exception.unknown_identifier == "Not-An-Exception");
}

TEST_CASE("a LicenseRef is well-formed", "[spdx]")
{
    auto const ref = check_spdx_expression("LicenseRef-Something-Nobody-Registered");
    CHECK(ref.well_formed);
    CHECK(ref.unknown_identifier.empty());

    auto const document = check_spdx_expression("DocumentRef-abc:LicenseRef-xyz AND MIT");
    CHECK(document.well_formed);
    CHECK(document.unknown_identifier.empty());
}
