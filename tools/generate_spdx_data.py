#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Adam Fidel
# SPDX-License-Identifier: MIT

"""Regenerate src/data/spdx_licenses.cpp from a pinned SPDX License List release.
"""

import argparse
import json
import pathlib
import shutil
import subprocess
import sys
import urllib.request

# The pinned upstream release
DEFAULT_TAG = "v3.28.0"

REPO = "spdx/license-list-data"
RAW = f"https://raw.githubusercontent.com/{REPO}"

REPO_ROOT = pathlib.Path(__file__).resolve().parent.parent
OUTPUT = REPO_ROOT / "src" / "data" / "spdx_licenses.cpp"

HEADER = """\
// SPDX-FileCopyrightText: 2026 Adam Fidel
// SPDX-License-Identifier: MIT
//
// SPDX-FileCopyrightText: Linux Foundation and SPDX contributors
// SPDX-License-Identifier: CC-BY-3.0

// GENERATED FILE. DO NOT EDIT.
//
// Regenerate with `just generate-spdx`, which runs tools/generate_spdx_data.py.
//
// Source: the SPDX License List, release @version@ of @date@, taken from
// https://github.com/@repo@ at tag @tag@. The list data is CC-BY-3.0; see
// LICENSES/CC-BY-3.0.txt.
//
// The pin is deliberate. An identifier added upstream after this release is not
// known here and draws a spurious finding, which is why bad-spdx-expression is a
// warning rather than an error (DECK.md section 7.1).

#include "spdx_licenses.hpp"

#include <algorithm>
#include <array>
#include <string_view>

namespace arcana::data
{

namespace
{

// Sorted ascending bytewise. Deprecated identifiers are included: they are on
// the list, so a deck naming one satisfies DECK.md section 7.1.
constexpr std::array<std::string_view, @license_count@> license_ids{
@license_rows@
};

// The exception identifiers usable to the right of a WITH operator.
constexpr std::array<std::string_view, @exception_count@> exception_ids{
@exception_rows@
};

static_assert(std::ranges::is_sorted(license_ids), "the generator must emit a sorted array");
static_assert(std::ranges::is_sorted(exception_ids), "the generator must emit a sorted array");

}  // namespace

bool is_spdx_license_id(std::string_view id) noexcept
{
    return std::ranges::binary_search(license_ids, id);
}

bool is_spdx_exception_id(std::string_view id) noexcept
{
    return std::ranges::binary_search(exception_ids, id);
}

}  // namespace arcana::data
"""


def fetch(tag: str, name: str) -> dict:
    url = f"{RAW}/{tag}/json/{name}.json"
    with urllib.request.urlopen(url, timeout=120) as response:  # noqa: S310
        return json.load(response)


def quoted(values: list[str]) -> str:
    return "\n".join(f'    "{value}",' for value in sorted(values))


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("tag", nargs="?", default=DEFAULT_TAG)
    args = parser.parse_args()

    clang_format = shutil.which("clang-format")
    if clang_format is None:
        print(
            "clang-format missing",
            file=sys.stderr,
        )
        return 1

    licenses = fetch(args.tag, "licenses")
    exceptions = fetch(args.tag, "exceptions")

    license_ids = sorted({entry["licenseId"] for entry in licenses["licenses"]})
    exception_ids = sorted(
        {entry["licenseExceptionId"] for entry in exceptions["exceptions"]}
    )

    if not license_ids or not exception_ids:
        print(f"{args.tag} carries no identifiers; refusing to write", file=sys.stderr)
        return 1

    text = HEADER
    for name, value in {
        "version": licenses["licenseListVersion"],
        "date": licenses["releaseDate"][:10],
        "repo": REPO,
        "tag": args.tag,
        "license_count": str(len(license_ids)),
        "license_rows": quoted(license_ids),
        "exception_count": str(len(exception_ids)),
        "exception_rows": quoted(exception_ids),
    }.items():
        text = text.replace(f"@{name}@", value)

    OUTPUT.write_text(text, encoding="utf-8")

    subprocess.run(
        [clang_format, "-i", f"--style=file:{REPO_ROOT / '.clang-format'}", str(OUTPUT)],
        check=True,
    )

    print(
        f"wrote {OUTPUT.relative_to(REPO_ROOT)}: "
        f"{len(license_ids)} licenses, {len(exception_ids)} exceptions "
        f"from {args.tag}"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
