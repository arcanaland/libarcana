# SPDX-FileCopyrightText: 2026 Adam Fidel
# SPDX-License-Identifier: MIT

arcana_add_test(dispatch_test LIBS arcana::loader)
arcana_add_test(loader_test LIBS arcana::loader)
arcana_add_test(names_test LIBS arcana::loader)
arcana_add_test(reader_test LIBS arcana::loader)
arcana_add_test(summary_test LIBS arcana::loader
    DEFINES REFERENCE_DECKS_DIR="${arcana_reference_decks_dir}")
