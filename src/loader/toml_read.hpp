// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#pragma once

#include <toml++/toml.hpp>

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace arcana::detail
{

// Reading a deck.toml is permissive throughout: a key of the wrong type, or a key
// that is not there at all, yields the fallback rather than an error. These are the
// primitives that permissiveness is built out of.

inline std::optional<std::string> get_string(toml::node_view<toml::node const> const& node)
{
    if (auto value = node.value<std::string>())
        return value;

    return std::nullopt;
}

inline std::string get_string_or(
    toml::node_view<toml::node const> const& node, std::string fallback = ""
)
{
    return node.value<std::string>().value_or(std::move(fallback));
}

inline std::vector<std::string> get_string_array(toml::node_view<toml::node const> const& node)
{
    std::vector<std::string> result;
    if (auto const* array = node.as_array())
    {
        for (auto const& element : *array)
            if (auto value = element.value<std::string>())
                result.push_back(*value);
    }
    return result;
}

// A table of string -> string, skipping any value that is not a string
inline std::unordered_map<std::string, std::string> get_string_map(
    toml::node_view<toml::node const> const& node
)
{
    std::unordered_map<std::string, std::string> result;

    if (auto const* t = node.as_table())
    {
        for (auto const& [key, value] : *t)
            if (auto s = value.value<std::string>())
                result.emplace(key.str(), *s);
    }

    return result;
}

}  // namespace arcana::detail
