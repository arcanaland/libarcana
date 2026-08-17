// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include <image_headers.hpp>
#include <image_signature.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <span>
#include <string_view>

using arcana::data::image_format;
using arcana::data::sniff_image_format;

namespace
{

// Wraps the bytes a signature test cares about
std::span<std::byte const> bytes_of(std::string_view s)
{
    return {reinterpret_cast<std::byte const*>(s.data()), s.size()};
}

}  // namespace

TEST_CASE("image formats are read from signature bytes", "[image_signature]")
{
    CHECK(sniff_image_format(bytes_of(arcana_test::png_header)) == image_format::png);

    // The formats whose headers carry bytes past the signature still sniff.
    CHECK(sniff_image_format(bytes_of(arcana_test::jpeg_header)) == image_format::jpeg);

    CHECK(sniff_image_format(bytes_of(arcana_test::webp_header)) == image_format::webp);

    CHECK(sniff_image_format(bytes_of("<svg xmlns=")) == image_format::unknown);
    CHECK(sniff_image_format(bytes_of("GIF89a")) == image_format::unknown);
    CHECK(sniff_image_format(bytes_of("")) == image_format::unknown);

    // A RIFF container that is not WebP such as a wav file.
    CHECK(sniff_image_format(bytes_of("RIFF****WAVEfmt ")) == image_format::unknown);

    // A truncated signature is not a signature.
    CHECK(
        sniff_image_format(bytes_of(arcana_test::png_header.substr(0, 4))) == image_format::unknown
    );
    CHECK(
        sniff_image_format(bytes_of(arcana_test::jpeg_header.substr(0, 2))) == image_format::unknown
    );
    CHECK(
        sniff_image_format(bytes_of(arcana_test::webp_header.substr(0, 11))) ==
        image_format::unknown
    );
}
