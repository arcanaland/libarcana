// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#pragma once

#include <string_view>

namespace arcana::data
{

// Membership in the vendored SPDX License List.
//
// The list is pinned at generation time.

// True when id is a licence identifier on the list
[[nodiscard]] bool is_spdx_license_id(std::string_view id) noexcept;

// True when id is an exception identifier
[[nodiscard]] bool is_spdx_exception_id(std::string_view id) noexcept;

}  // namespace arcana::data
