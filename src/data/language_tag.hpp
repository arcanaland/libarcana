// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#pragma once

#include <string>
#include <string_view>

namespace arcana::data
{

// True when `tag` is a well-formed BCP 47 language tag.
[[nodiscard]] bool is_well_formed_language_tag(std::string_view tag);

// the shortest available ISO 639 subtag, lowercase language, titlecase script,
// uppercase region.
[[nodiscard]] std::string canonicalize_language_tag(std::string_view tag);

// True when `tag` is well-formed and already in canonical form.
[[nodiscard]] bool is_canonical_language_tag(std::string_view tag);

}  // namespace arcana::data
