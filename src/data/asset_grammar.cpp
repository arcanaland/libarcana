// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include "asset_grammar.hpp"

#include "ascii.hpp"
#include "text.hpp"

#include <algorithm>
#include <charconv>
#include <filesystem>
#include <optional>
#include <string_view>
#include <system_error>

namespace arcana::data
{

namespace
{

// The <height> of an h<n>/ root or the <lines> of an ansi<n>/ root
std::optional<int> parse_root_size(std::string_view digits) noexcept
{
    if (digits.empty() || !std::ranges::all_of(digits, is_digit))
        return std::nullopt;

    if (digits.front() == '0')
        return std::nullopt;

    int value = 0;
    auto const [_, ec] = std::from_chars(digits.data(), digits.data() + digits.size(), value);
    if (ec != std::errc{} || value <= 0)
        return std::nullopt;

    return value;
}

}  // namespace

std::optional<image_root_name> parse_image_root(std::string_view name) noexcept
{
    if (name == "scalable")
        return image_root_name{.kind = image_kind::scalable, .size = std::nullopt};

    if (name == "surrogate")
        return image_root_name{.kind = image_kind::surrogate, .size = std::nullopt};

    if (name.starts_with("h"))
        if (auto const height = parse_root_size(name.substr(1)))
            return image_root_name{.kind = image_kind::raster, .size = height};

    if (name.starts_with("ansi"))
        if (auto const lines = parse_root_size(name.substr(4)))
            return image_root_name{.kind = image_kind::ansi, .size = lines};

    return std::nullopt;
}

std::optional<int> chain_rank(image_kind kind, std::string_view extension) noexcept
{
    switch (kind)
    {
        case image_kind::scalable:
            return extension == "svg" ? std::optional{0} : std::nullopt;

        case image_kind::raster:
            if (extension == "png")
                return 0;
            if (extension == "webp")
                return 1;
            if (extension == "avif")
                return 2;
            // the spec ranks these together
            if (extension == "jpeg" || extension == "jpg")
                return 3;
            return std::nullopt;

        case image_kind::ansi:
            // ANSI files take any extension or none
            return 0;

        case image_kind::surrogate:
            return extension == "toml" ? std::optional{0} : std::nullopt;
    }

    return std::nullopt;
}

bool is_baseline_extension(std::string_view extension) noexcept
{
    return extension == "png" || extension == "webp" || extension == "jpeg" || extension == "jpg";
}

asset_filename split_asset_filename(std::string_view filename) noexcept
{
    asset_filename result;

    auto const last_dot = filename.rfind('.');
    result.stem = last_dot == std::string_view::npos ? filename : filename.substr(0, last_dot);
    if (last_dot != std::string_view::npos)
        result.extension = filename.substr(last_dot + 1);

    auto const first_dot = result.stem.find('.');
    result.base = result.stem.substr(0, first_dot);
    if (first_dot != std::string_view::npos)
        result.variant_key = result.stem.substr(first_dot + 1);

    return result;
}

path_parts components_of(std::filesystem::path const& relative) noexcept
{
    // We'll need to update this strategy when we support Windows
    static_assert(std::filesystem::path::preferred_separator == '/');

    path_parts found;
    for (auto const one : pieces(std::string_view{relative.native()}, '/'))
    {
        if (found.size == found.parts.size())
            return {};

        found.parts[found.size++] = one;
    }

    return found;
}

}  // namespace arcana::data
