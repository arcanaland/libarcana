// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>
#include <string>

namespace arcana
{

enum class error_code : std::uint8_t
{
    not_found,
    parse_error,
    io_error,
    invalid_argument,
};

struct error
{
    error_code code;
    std::string message;
};

}  // namespace arcana
