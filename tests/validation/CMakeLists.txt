# SPDX-FileCopyrightText: 2026 Adam Fidel
# SPDX-License-Identifier: MIT

arcana_add_test(ansi_test LIBS arcana::arcana)
arcana_add_test(backs_test LIBS arcana::arcana)
arcana_add_test(cards_test LIBS arcana::arcana)
# The entry index is read directly, so validation-core's objects are named
# alongside the archive that otherwise carries them.
arcana_add_test(facts_test LIBS arcana::validation-core arcana::arcana)
arcana_add_test(ids_test LIBS arcana::arcana)
arcana_add_test(images_test LIBS arcana::arcana)
