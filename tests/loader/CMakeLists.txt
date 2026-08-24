# SPDX-FileCopyrightText: 2026 Adam Fidel
# SPDX-License-Identifier: MIT

# dispatch_test and reader_test drive the public load_deck over a deck on disk,
# so arcana::arcana already carries everything they include.
arcana_add_test(dispatch_test LIBS arcana::arcana)
arcana_add_test(reader_test LIBS arcana::arcana)

arcana_add_test(loader_test LIBS arcana::loader-v1 arcana::arcana)
# The one true leaf here: OBJECT libraries do not propagate objects, so the
# model and its identifier tables are named explicitly rather than arrived at.
arcana_add_test(names_test LIBS arcana::loader-core arcana::model arcana::identifiers)
arcana_add_test(summary_test LIBS arcana::loader arcana::arcana
    DEFINES REFERENCE_DECKS_DIR="${arcana_reference_decks_dir}")
