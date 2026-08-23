# SPDX-FileCopyrightText: 2026 Adam Fidel
# SPDX-License-Identifier: MIT

# Each of these links exactly the leaf it exercises, so a dependency leak in
# that leaf is a link error here rather than an invisible pass.
arcana_add_test(asset_grammar_test LIBS arcana::vocab)
arcana_add_test(identifiers_test LIBS arcana::identifiers)
arcana_add_test(image_signature_test LIBS arcana::imagesig)
arcana_add_test(language_tag_test LIBS arcana::bcp47)
arcana_add_test(spdx_test LIBS arcana::spdx)
arcana_add_test(tables_test LIBS arcana::bcp47 arcana::spdx)
arcana_add_test(uri_test LIBS arcana::uri)
