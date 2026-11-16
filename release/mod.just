# SPDX-FileCopyrightText: 2026 Adam Fidel
# SPDX-License-Identifier: MIT

set working-directory := '..'

# Print the release phases.
[no-exit-message]
default:
    @./tools/release.sh

# phase 1: bump the version
[no-exit-message]
prepare version:
    ./tools/release.sh prepare {{version}}

# phase 2: commit the release + create the tag
[no-exit-message]
tag:
    ./tools/release.sh tag

# phase 3: push main and the tag
[no-exit-message]
push:
    ./tools/release.sh push

# phase 4: create the GitHub release (which uploads to PyPI)
[no-exit-message]
publish:
    ./tools/release.sh publish

# optional: watch CI on HEAD, after push and before publish
[no-exit-message]
ci:
    ./tools/release.sh ci

# optional: build every artifact and upload to TestPyPI
[no-exit-message]
dry-run:
    ./tools/release.sh dry-run
