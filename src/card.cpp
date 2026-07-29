// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include <arcana/card.hpp>

#include <algorithm>
#include <array>
#include <charconv>
#include <cstdlib>
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

std::vector<std::string_view> split(std::string_view text, char delim)
{
    std::vector<std::string_view> parts;
    std::size_t start = 0;
    while (start <= text.size())
    {
        auto const pos = text.find(delim, start);
        if (pos == std::string_view::npos)
        {
            parts.push_back(text.substr(start));
            break;
        }
        parts.push_back(text.substr(start, pos - start));
        start = pos + 1;
    }
    return parts;
}

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

std::expected<card_id, error> parse_card_id(std::string_view canonical_id)
{
    auto const parts = split(canonical_id, '.');

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
        // Two digits is always a standard major, so "major_arcana.22" is out of range
        // rather than a custom card whose id happens to be "22".
        if (parts[1].size() == 2 &&
            std::ranges::all_of(parts[1], [](unsigned char c) { return std::isdigit(c) != 0; }))
        {
            auto const number = parse_major_number(parts[1]);
            if (!number)
                return std::unexpected(number.error());
            return card_id::standard_major(*number);
        }

        if (!is_valid_identifier(parts[1]))
            return parse_error_for("not a canonical card id");
        return card_id::custom_major(std::string(parts[1]));
    }

    if (parts.size() == 3 && parts[0] == "minor_arcana")
    {
        // A canonical suit commits the id to the standard 56: a deck cannot add a card to
        // an existing suit, so an unrecognised rank there is a malformed id.
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

std::optional<image_variant> best_variant_for_height(
    std::vector<image_variant> const& variants, int target_height
)
{
    auto sized = variants | std::views::filter([](image_variant const& variant)
                                               { return variant.height.has_value(); });

    // Prefer a variant that meets the target, then the one closest to it.
    // Among equally-close candidates the smaller one.
    auto const rank_of = [target_height](image_variant const& variant)
    {
        int const height = *variant.height;
        return std::tuple{height < target_height, std::abs(height - target_height), height};
    };

    auto const best = std::ranges::min_element(sized, {}, rank_of);

    if (best == std::ranges::end(sized))
        return std::nullopt;

    return *best;
}

std::string card::canonical_id() const
{
    return id.to_canonical();
}

std::optional<image_variant> card::best_image_for_height(int target_height) const
{
    return best_variant_for_height(images, target_height);
}

}  // namespace arcana
