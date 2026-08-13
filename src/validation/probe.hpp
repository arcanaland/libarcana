// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

// Bounded reads of a file's contents. Everything here does I/O, which is why it
// is separated from facts.hpp: the `phase::document` rules must never reach it.

#pragma once

#include <filesystem>

namespace arcana::validation
{

// True where an ESC turns up before a NUL in the head of the file. A file that
// is unreadable, empty or binary reads as false.
[[nodiscard]] bool contains_ansi_escapes(std::filesystem::path const& file);

// True where the file's leading bytes are a format every application can
// decode, meaning PNG or JPEG.
[[nodiscard]] bool is_baseline_image_format(std::filesystem::path const& file);

}  // namespace arcana::validation
