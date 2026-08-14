// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#pragma once

#include <filesystem>

namespace arcana::validation
{

// If an ESC appears in the head of the file.
[[nodiscard]] bool contains_ansi_escapes(std::filesystem::path const& file);

// file's leading bytes are one of the baseline formats: PNG, JPEG or WebP
[[nodiscard]] bool is_baseline_image_format(std::filesystem::path const& file);

}  // namespace arcana::validation
