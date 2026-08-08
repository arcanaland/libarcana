// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#pragma once

#include <string>
#include <string_view>

namespace arcana::data
{

// The language tag rules of DECK.md section 6.1.

// True when `tag` is a well-formed BCP 47 language tag.
//
// Well-formed, not valid: section 6.1 asks for conformance to the RFC 5646
// grammar and nothing more, so no subtag is looked up in the IANA registry and
// no registry ships here. The grandfathered tags RFC 5646 lists are accepted
// because the grammar admits them; the irregular ones are matched by name.
[[nodiscard]] bool is_well_formed_language_tag(std::string_view tag);

// `tag` in the canonical form section 6.1 describes: the shortest available ISO
// 639 subtag, lowercase language, titlecase script, uppercase region, and lower
// case for everything else.
//
// Empty when `tag` is not well-formed. A grandfathered tag is returned as it
// was given: rewriting one to its preferred value needs the IANA registry.
[[nodiscard]] std::string canonicalize_language_tag(std::string_view tag);

// True when `tag` is well-formed and already in that canonical form.
[[nodiscard]] bool is_canonical_language_tag(std::string_view tag);

}  // namespace arcana::data
