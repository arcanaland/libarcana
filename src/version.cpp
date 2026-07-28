// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include <arcana/version.hpp>

namespace arcana
{

std::string_view library_version() noexcept
{
    return version;
}

}  // namespace arcana
