// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#pragma once

#include <string_view>

namespace arcana::data
{

// True when `s` is an absolute URL with a scheme of http or https, which is
// what DECK.md section 4.1.1 requires of a link's url.
//
// Well-formedness only. Nothing is fetched and no host is resolved.
[[nodiscard]] bool is_absolute_http_url(std::string_view s) noexcept;

}  // namespace arcana::data
