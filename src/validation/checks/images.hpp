// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

// Checks for the `images` area.
//
// `aspect-ratio-mismatch` is deliberately absent: it needs an image decoder,
// and no image decoder enters this tree.

#pragma once

#include "../context.hpp"

namespace arcana::validation
{

void check_duplicate_chain_extension(check_context const& ctx);

void check_ignored_image_root_lookalike(check_context const& ctx);

void check_raster_outside_image_root(check_context const& ctx);

void check_stem_case_collision(check_context const& ctx);

void check_svg_outside_scalable(check_context const& ctx);

}  // namespace arcana::validation
