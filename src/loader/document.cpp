// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#include "document.hpp"

#include "manifest.hpp"

#include <arcana/deck.hpp>

#include <format>
#include <sstream>
#include <string>
#include <utility>

namespace arcana
{

namespace detail
{

std::expected<std::shared_ptr<deck_document const>, error> read_deck_document(
    std::filesystem::path const& deck_directory
)
{
    std::filesystem::path const toml_path = deck_directory / deck_manifest_filename;

    auto parsed = toml::parse_file(toml_path.string());
    if (!parsed)
    {
        return std::unexpected(
            error{
                .code = error_code::parse_error,
                .message = std::format(
                    "failed to parse {}: {}", toml_path.string(), parsed.error().description()
                )
            }
        );
    }

    auto document = std::make_shared<deck_document>(std::move(parsed).table());
    if (document->table["deck"].as_table() == nullptr)
    {
        return std::unexpected(
            error{
                .code = error_code::parse_error,
                .message = std::format("{} has no [deck] table", toml_path.string())
            }
        );
    }

    return document;
}

}  // namespace detail

std::string deck::source_toml() const
{
    if (!document_)
        return {};

    std::ostringstream out;
    out << document_->table;
    return std::move(out).str();
}

}  // namespace arcana
