// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT

#pragma once

// Hand-written scanners for the grammars the deck specification defines.
//
// No regular-expression engine is involved, in the standard library or out of
// it. Every grammar below is a character-class scan over ASCII, which a
// hand-written scanner does in a few lines and reports better errors for.
//
// This header is the umbrella over all of them. Prefer including the one you
// need: each is a separate translation unit and only the SPDX expression parser
// pulls in the vendored license list.

#include "identifiers.hpp"      // NOLINT(misc-include-cleaner)
#include "image_signature.hpp"  // NOLINT(misc-include-cleaner)
#include "language_tag.hpp"     // NOLINT(misc-include-cleaner)
#include "spdx_expression.hpp"  // NOLINT(misc-include-cleaner)
#include "uri.hpp"              // NOLINT(misc-include-cleaner)
