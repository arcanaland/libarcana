// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#pragma once

#include <string_view>

namespace arcana_test
{

// The leading bytes a baseline formats

inline constexpr std::string_view png_header = "\x89PNG\r\n\x1a\n";

inline constexpr std::string_view jpeg_header = "\xff\xd8\xff\xe0JFIF";

// A RIFF container: the tag, a four byte size (stubbed here) and the WEBP form.
inline constexpr std::string_view webp_header = "RIFF****WEBPVP8 ";

}  // namespace arcana_test
