// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include "spec.hpp"

#include <arcana/deck.hpp>

#include <toml++/toml.hpp>

#include <cstdint>
#include <string_view>

namespace arcana::validation
{

std::uint8_t major_of(deck const& d) noexcept
{
    return schema_major(d.metadata).value_or(current_schema_major);
}

std::string_view card_back_designs_key(std::uint8_t major) noexcept
{
    return major < 2 ? "variants" : "designs";
}

toml::table const* card_back_designs(toml::table const& doc, std::uint8_t major)
{
    return doc["card_backs"][card_back_designs_key(major)].as_table();
}

}  // namespace arcana::validation
