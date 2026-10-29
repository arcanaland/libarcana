#!/bin/bash
# SPDX-FileCopyrightText: 2026 Adam Fidel
# SPDX-License-Identifier: MIT

# Fail if code or build files name a knowledge-base doc (RFC-…, ADR-…, TASK-…).
# With no arguments, scan the working tree; with arguments, scan those staged files.
set -euo pipefail

pattern='(^|[^[:alnum:]_])(RFC|ADR|TASK)-[0-9]+'

if [ $# -eq 0 ]; then
  source=(--untracked)
  set -- '*.cpp' '*.hpp' '*.in' '*.cmake' '*CMakeLists.txt' \
    '*.py' '*.sh' '*.just' 'justfile' 'Containerfile'
else
  source=(--cached)
fi

if git grep -nE ${source[@]+"${source[@]}"} "$pattern" -- "$@"; then
  echo "slop detected"
  exit 1
fi
