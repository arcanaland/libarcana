// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

// Checks for the `ansi` area.

#pragma once

#include "../context.hpp"

namespace arcana::validation
{

void check_ansi_outside_image_root(check_context const& ctx);

}  // namespace arcana::validation
