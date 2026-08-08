// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#pragma once

#include <string_view>

namespace arcana::data
{

[[nodiscard]] bool is_absolute_http_url(std::string_view s) noexcept;

}  // namespace arcana::data
