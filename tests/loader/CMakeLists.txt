# SPDX-FileCopyrightText: 2026 Adam Fidel
# SPDX-License-Identifier: MIT

# These link arcana::loader for its headers, toml++ and TOML_EXCEPTIONS, on top of
# arcana::arcana for the model symbols the loader calls into. libarcana also contains
# the loader's objects, but arcana_add_test puts LIBS ahead of arcana::arcana on the
# link line, so those archive members are never extracted and nothing is defined twice.
arcana_add_test(loader_test LIBS arcana::loader)
arcana_add_test(names_test LIBS arcana::loader)
arcana_add_test(summary_test LIBS arcana::loader)
