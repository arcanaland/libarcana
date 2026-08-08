// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include <uri.hpp>

#include <catch2/catch_test_macros.hpp>

using arcana::data::is_absolute_http_url;

TEST_CASE("link URLs are absolute http or https", "[uri]")
{
    CHECK(is_absolute_http_url("https://example.com/shop/the-deck"));
    CHECK(is_absolute_http_url("http://example.com"));
    CHECK(is_absolute_http_url("HTTPS://example.com/about"));
    CHECK(is_absolute_http_url("https://example.com/a?b=c#d"));

    CHECK_FALSE(is_absolute_http_url(""));
    CHECK_FALSE(is_absolute_http_url("example.com"));
    CHECK_FALSE(is_absolute_http_url("/shop/the-deck"));
    CHECK_FALSE(is_absolute_http_url("ftp://example.com"));
    CHECK_FALSE(is_absolute_http_url("mailto:someone@example.com"));
    CHECK_FALSE(is_absolute_http_url("https://"));
    CHECK_FALSE(is_absolute_http_url("https:///path"));
    CHECK_FALSE(is_absolute_http_url("https://example.com/a b"));
}
