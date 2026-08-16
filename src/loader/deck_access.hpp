// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#pragma once

#include "document.hpp"

#include <arcana/deck.hpp>

#include <memory>
#include <string>
#include <unordered_map>

namespace arcana::detail
{

// The one door into deck's private state
//
// deck names this and nothing else as a friend, so neither reader is
// privileged over the other in a public header
struct deck_access
{
    [[nodiscard]] static std::unordered_map<std::string, std::string>& rank_names(deck& d) noexcept
    {
        return d.rank_names_;
    }

    [[nodiscard]] static std::shared_ptr<deck_document const>& document(deck& d) noexcept
    {
        return d.document_;
    }
};

}  // namespace arcana::detail
