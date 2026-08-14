// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include <arcana/card.hpp>

#include "data/text.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <cstdlib>
#include <expected>
#include <format>
#include <ranges>
#include <tuple>

namespace arcana
{

namespace
{

constexpr std::array<std::string_view, 4> suit_names{"wands", "cups", "swords", "pentacles"};

constexpr std::array<std::string_view, 14> rank_names{"ace",  "two",    "three", "four", "five",
                                                      "six",  "seven",  "eight", "nine", "ten",
                                                      "page", "knight", "queen", "king"};

}  // namespace

std::string_view to_string(suit s) noexcept
{
    return suit_names[static_cast<std::size_t>(s)];
}

std::string_view to_string(rank r) noexcept
{
    return rank_names[static_cast<std::size_t>(r)];
}

std::optional<suit> suit_from_string(std::string_view text) noexcept
{
    auto const* const it = std::ranges::find(suit_names, text);

    if (it == suit_names.end())
        return std::nullopt;

    return static_cast<suit>(std::distance(suit_names.begin(), it));
}

std::optional<rank> rank_from_string(std::string_view text) noexcept
{
    auto const* const it = std::ranges::find(rank_names, text);

    if (it == rank_names.end())
        return std::nullopt;

    return static_cast<rank>(std::distance(rank_names.begin(), it));
}

namespace
{

std::expected<int, error> parse_major_number(std::string_view text)
{
    if (text.size() != 2)
    {
        return std::unexpected(
            error{
                .code = error_code::parse_error,
                .message = std::format("major arcana number must be 2 digits: '{}'", text)
            }
        );
    }

    int value = 0;
    auto const [ptr, ec] = std::from_chars(text.data(), text.data() + text.size(), value);

    if (ec != std::errc{} || ptr != text.data() + text.size() || value < 0 ||
        value > max_major_arcana_number)
    {
        return std::unexpected(
            error{
                .code = error_code::parse_error,
                .message = std::format("invalid major arcana number: '{}'", text)
            }
        );
    }

    return value;
}

}  // namespace

card_id card_id::standard_major(int number)
{
    return card_id{.cls = card_class::standard_major, .number = number};
}

card_id card_id::standard_minor(suit s, rank r)
{
    return card_id{.cls = card_class::standard_minor, .standard_suit = s, .standard_rank = r};
}

card_id card_id::custom_major(std::string id)
{
    return card_id{.cls = card_class::custom_major, .custom_id = std::move(id)};
}

card_id card_id::custom_minor(std::string suit_key, std::string id)
{
    return card_id{
        .cls = card_class::custom_minor, .suit_key = std::move(suit_key), .custom_id = std::move(id)
    };
}

bool card_id::is_major() const noexcept
{
    return cls == card_class::standard_major || cls == card_class::custom_major;
}

arcana_kind card_id::kind() const noexcept
{
    return is_major() ? arcana_kind::major_arcana : arcana_kind::minor_arcana;
}

bool card_id::is_custom() const noexcept
{
    return cls == card_class::custom_major || cls == card_class::custom_minor;
}

std::string card_id::to_canonical() const
{
    switch (cls)
    {
        case card_class::standard_major:
            return std::format("major_arcana.{:02d}", number);
        case card_class::custom_major:
            return std::format("major_arcana.{}", custom_id);
        case card_class::standard_minor:
            return std::format(
                "minor_arcana.{}.{}", to_string(standard_suit), to_string(standard_rank)
            );
        case card_class::custom_minor:
            return std::format("minor_arcana.{}.{}", suit_key, custom_id);
    }

    return {};
}

bool is_valid_identifier(std::string_view text) noexcept
{
    if (text.empty())
        return false;

    return std::ranges::all_of(
        text,
        [](unsigned char c) { return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_'; }
    );
}

std::expected<card_id, error> card_id::parse(std::string_view canonical_id)
{
    auto const parts = pieces(canonical_id, '.') | std::ranges::to<std::vector>();

    auto const parse_error_for = [canonical_id](std::string_view what)
    {
        return std::unexpected(
            error{
                .code = error_code::parse_error,
                .message = std::format("{}: '{}'", what, canonical_id)
            }
        );
    };

    if (parts.size() == 2 && parts[0] == "major_arcana")
    {
        if (parts[1].size() == 2 &&
            std::ranges::all_of(parts[1], [](unsigned char c) { return std::isdigit(c) != 0; }))
        {
            auto const number = parse_major_number(parts[1]);
            if (!number)
                return std::unexpected(number.error());

            return card_id::standard_major(*number);
        }

        // maybe a custom
        if (!is_valid_identifier(parts[1]))
            return parse_error_for("not a canonical card id");

        return card_id::custom_major(std::string(parts[1]));
    }

    if (parts.size() == 3 && parts[0] == "minor_arcana")
    {
        if (auto const parsed_suit = suit_from_string(parts[1]))
        {
            auto const parsed_rank = rank_from_string(parts[2]);
            if (!parsed_rank)
                return parse_error_for("invalid minor arcana canonical id");

            return card_id::standard_minor(*parsed_suit, *parsed_rank);
        }

        if (!is_valid_identifier(parts[1]) || !is_valid_identifier(parts[2]))
            return parse_error_for("invalid minor arcana canonical id");

        return card_id::custom_minor(std::string(parts[1]), std::string(parts[2]));
    }

    return parse_error_for("not a canonical card id");
}

namespace
{

std::optional<card_image> best_of_kind(
    std::vector<card_image> const& images, image_kind kind, int target
)
{
    auto const size_field = (kind == image_kind::ansi) ? &card_image::lines : &card_image::height;

    // Raster prefers the smallest at or above the target
    // ANSI prefers the largest at or below the target
    bool const prefer_at_least = (kind != image_kind::ansi);

    auto candidates =
        images | std::views::filter([kind, size_field](card_image const& image)
                                    { return image.kind == kind && (image.*size_field); });

    // Right side of the target first, then closest.
    // break ties toward the preferred side
    auto const rank_of = [target, size_field, prefer_at_least](card_image const& image)
    {
        int const size = *(image.*size_field);
        bool const wrong_side = prefer_at_least ? (size < target) : (size > target);
        return std::tuple{wrong_side, std::abs(size - target), prefer_at_least ? size : -size};
    };

    auto const best = std::ranges::min_element(candidates, {}, rank_of);

    if (best == std::ranges::end(candidates))
        return std::nullopt;

    return *best;
}

}  // namespace

std::string card::canonical_id() const
{
    return id.to_canonical();
}

std::optional<card_image> card::scalable_image() const
{
    auto const found = std::ranges::find(images, image_kind::scalable, &card_image::kind);

    if (found == images.end())
        return std::nullopt;

    return *found;
}

std::optional<card_image> card::best_raster_for_height(int target_height) const
{
    return best_of_kind(images, image_kind::raster, target_height);
}

std::optional<card_image> card::best_ansi_for_lines(int target_lines) const
{
    return best_of_kind(images, image_kind::ansi, target_lines);
}

}  // namespace arcana
