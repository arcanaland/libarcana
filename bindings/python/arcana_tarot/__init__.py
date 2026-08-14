# SPDX-FileCopyrightText: 2026 Adam Fidel
# SPDX-License-Identifier: MIT

"""Python binding for libarcana, an implementation of the Arcana Land tarot specs.

The C++ library is `arcana` everywhere — namespace, CMake target, headers. Python
is the one exception (ADR-025): the distribution is `arcana-tarot` and the import
is `arcana_tarot`, because `arcana` on PyPI belongs to an unrelated project.

Everything below comes from `_core`, the single nanobind extension. There is
deliberately only one: further specs (esoterica, spreads) become pure-Python
submodules over this surface rather than additional extensions, so `libarcana.a`
is linked into the wheel exactly once.
"""

from ._core import (
    __version__,
    arcana_kind,
    card,
    card_class,
    card_id,
    card_image,
    deck,
    deck_error,
    deck_library,
    deck_library_path,
    deck_metadata,
    deck_summary,
    deck_summary_view,
    error,
    error_code,
    image_kind,
    is_valid_identifier,
    library_options,
    load_deck,
    malformed_deck,
    rank,
    rank_from_string,
    suit,
    suit_from_string,
    suit_info,
    to_string,
)

__all__ = [
    "__version__",
    "arcana_kind",
    "card",
    "card_class",
    "card_id",
    "card_image",
    "deck",
    "deck_error",
    "deck_library",
    "deck_library_path",
    "deck_metadata",
    "deck_summary",
    "deck_summary_view",
    "error",
    "error_code",
    "image_kind",
    "is_valid_identifier",
    "library_options",
    "load_deck",
    "malformed_deck",
    "rank",
    "rank_from_string",
    "suit",
    "suit_from_string",
    "suit_info",
    "to_string",
]
