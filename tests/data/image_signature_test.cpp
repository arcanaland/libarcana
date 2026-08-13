// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include <image_signature.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <span>
#include <string_view>

using arcana::data::image_format;
using arcana::data::sniff_image_format;

namespace
{

// Wraps the bytes a signature test cares about, which is all sniffing reads.
std::span<std::byte const> bytes_of(std::string_view s)
{
    return {reinterpret_cast<std::byte const*>(s.data()), s.size()};
}

}  // namespace

TEST_CASE("image formats are read from signature bytes", "[image_signature]")
{
    CHECK(sniff_image_format(bytes_of("\x89PNG\r\n\x1a\n\x00\x00")) == image_format::png);
    CHECK(sniff_image_format(bytes_of("\xff\xd8\xff\xe0JFIF")) == image_format::jpeg);

    // WebP became a baseline format at DECK.md `29edd24`. The form type sits four
    // bytes past the RIFF tag, behind the chunk size, which sniffing ignores. The
    // filler avoids NUL, which would truncate the literal these cases are built from.
    CHECK(sniff_image_format(bytes_of("RIFF****WEBPVP8 ")) == image_format::webp);

    CHECK(sniff_image_format(bytes_of("<svg xmlns=")) == image_format::unknown);
    CHECK(sniff_image_format(bytes_of("GIF89a")) == image_format::unknown);
    CHECK(sniff_image_format(bytes_of("")) == image_format::unknown);

    // A RIFF container that is not WebP, such as a wav file.
    CHECK(sniff_image_format(bytes_of("RIFF****WAVEfmt ")) == image_format::unknown);

    // A truncated signature is not a signature.
    CHECK(sniff_image_format(bytes_of("\x89PNG")) == image_format::unknown);
    CHECK(sniff_image_format(bytes_of("\xff\xd8")) == image_format::unknown);
    CHECK(sniff_image_format(bytes_of("RIFF****WEB")) == image_format::unknown);
}
