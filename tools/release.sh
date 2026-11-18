#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 Adam Fidel
# SPDX-License-Identifier: MIT

# Release arcana-tarot in four phases:
#
#   tools/release.sh prepare X.Y.Z   bump the version, add a CHANGELOG skeleton, stop
#   <write the notes in CHANGELOG.md, review the diff>
#   tools/release.sh tag             check the notes, commit, create the tag
#   tools/release.sh push            push main and the tag, atomically
#   tools/release.sh publish         create the GitHub release, which uploads to PyPI
#

set -euo pipefail

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

CMAKELISTS="CMakeLists.txt"
VERSION_TEST="tests/version_test.cpp"
CHANGELOG="CHANGELOG.md"
DIST="arcana-tarot"
BRANCH="main"

die() {
  echo -e "${RED}== $1${NC}" >&2
  exit 1
}

note() {
  echo -e "${YELLOW}== $1${NC}"
}

ok() {
  echo -e "${GREEN}== $1${NC}"
}

confirm() {
  echo -n "$1 (y/N): "
  read -r answer
  [ "$answer" = "y" ] || [ "$answer" = "Y" ] || die "aborted."
}

usage() {
  cat <<'EOF'
usage: release.sh prepare X.Y.Z   bump the version and add a CHANGELOG.md skeleton, then stop
       release.sh tag             validate the notes, commit the bump, create the tag
       release.sh push            push main and the tag, atomically
       release.sh publish         create the GitHub release; the workflow uploads to PyPI

optional, at any point:
       release.sh ci              watch CI on HEAD (run after push, before publish)
       release.sh dry-run         build every artifact and upload to TestPyPI

macOS wheel dry run, uploaded to TestPyPI:
       gh workflow run macos-wheel.yml -f ref=main -f repository=testpypi

Between prepare and tag, write the notes under the new heading in CHANGELOG.md.
The template is the HTML comment at the top of that file.
EOF
}

# ---------------------------------------------------------------- helpers ----

require_repo_root() {
  [ -f "$CMAKELISTS" ] && [ -f "$CHANGELOG" ] ||
    die "run this from the repository root ($CMAKELISTS or $CHANGELOG not found)"
}

# The version lines inside project(arcana ...), which put each keyword on its
# own line. cmake_minimum_required(VERSION ...) does not start with whitespace.
project_version() {
  sed -nE 's/^[[:space:]]+VERSION[[:space:]]+([0-9][0-9.]*).*/\1/p' "${1:-$CMAKELISTS}" | head -1
}

compat_version() {
  sed -nE 's/^[[:space:]]+COMPAT_VERSION[[:space:]]+([0-9][0-9.]*).*/\1/p' "$CMAKELISTS" | head -1
}

test_version() {
  sed -nE 's/.*library_version\(\) == "([^"]*)".*/\1/p' "$VERSION_TEST" | head -1
}

is_semver() {
  [[ "$1" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]
}

# True if $1 is strictly greater than $2.
version_gt() {
  [ "$1" != "$2" ] && [ "$(printf '%s\n%s\n' "$1" "$2" | sort -V | tail -1)" = "$1" ]
}

# Before 1.0 every minor is incompatible, so COMPAT_VERSION is always X.Y.0:
# a minor bump moves it and a patch bump leaves it where it is.
expected_compat() {
  local major minor _
  IFS=. read -r major minor _ <<<"$1"
  echo "$major.$minor.0"
}

pypi_latest() {
  curl -fsS --max-time 10 "https://pypi.org/pypi/$DIST/json" 2>/dev/null |
    python3 -c 'import json, sys; print(json.load(sys.stdin)["info"]["version"])' 2>/dev/null
}

# The CHANGELOG section for a version, heading included, up to the next
# "## [" heading. Lines inside an HTML comment are skipped, so the template is
# never mistaken for a release. Assumes comment markers start or end their lines.
changelog_section() {
  awk -v ver="$1" '
    incomment { if (/-->/) incomment = 0; next }
    /<!--/ { if (!/-->/) incomment = 1; next }
    inblock && /^## \[/ { exit }
    index($0, "## [" ver "]") == 1 { inblock = 1 }
    inblock { print }
  ' "${2:-$CHANGELOG}"
}

# The section without its heading and surrounding blank lines: the release body.
changelog_body() {
  changelog_section "$@" | tail -n +2 | sed -e '/./,$!d' | tac | sed -e '/./,$!d' | tac
}

require_on_main() {
  local current
  current=$(git rev-parse --abbrev-ref HEAD)
  [ "$current" = "$BRANCH" ] ||
    die "on '$current', not '$BRANCH'. A release commit goes straight to $BRANCH:
   a squash merge would leave the tag on a commit that never reaches it."
}

# HEAD must contain everything on origin. Otherwise the tag sits on a commit
# the rebase will rewrite, and a pushed tag is stranded off origin/main.
require_up_to_date() {
  echo "== fetching origin..."
  git fetch --quiet origin || die "could not fetch origin."

  local behind
  behind=$(git rev-list --count "HEAD..origin/$BRANCH")
  if [ "$behind" -ne 0 ]; then
    die "$BRANCH is behind origin/$BRANCH by $behind commit(s).
   Something landed since this release started. Rebase onto it and
   start over from 'prepare':

       git rebase origin/$BRANCH

   Do not tag or push until $BRANCH contains origin/$BRANCH."
  fi
}

tag_on_origin() {
  git ls-remote --tags --exit-code origin "refs/tags/$1" >/dev/null 2>&1
}

refuse_existing_tag() {
  local tag=$1 where=$2
  die "$tag already exists $where. This script never deletes a tag.
   If that tag is wrong and was never published to PyPI, delete it by hand:

       git tag -d $tag
       git push origin :refs/tags/$tag     # only if it reached origin"
}

# The newest run of a workflow on a commit, printed with the given jq template.
workflow_run_for() {
  gh run list --commit "$2" --workflow "$1" --limit 1 \
    --json databaseId,status,conclusion,url --jq ".[0] | select(.) | $3" 2>/dev/null
}

ci_run_for() {
  workflow_run_for ci.yml "$@"
}

report_macos_floor() {
  if ! command -v gh >/dev/null; then
    note "gh is not installed; macOS floor check not reported"
    return
  fi

  local sha short run
  sha=$(git rev-parse HEAD)
  short=$(git rev-parse --short HEAD)
  run=$(workflow_run_for macos-floor.yml "$sha" '"\(.conclusion) \(.url)"' || true)
  if [ "${run%% *}" = "success" ]; then
    ok "macOS floor check passed on $short: ${run#* }"
    return
  fi

  if [ -n "$run" ]; then
    note "macOS floor check on $short has not passed: $run"
  else
    note "no macOS floor check has run on $short"
  fi

  local green
  green=$(gh run list --workflow macos-floor.yml --branch "$BRANCH" --status success --limit 1 \
    --json headSha --jq '.[0].headSha // empty' 2>/dev/null || true)
  if [ -n "$green" ] && git cat-file -e "$green^{commit}" 2>/dev/null; then
    note "newest green: $(git rev-parse --short "$green"), $(git rev-list --count "$green..HEAD") commit(s) behind HEAD"
  else
    note "no green macOS floor check found on $BRANCH"
  fi
  note "start one with: gh workflow run macos-floor.yml"
}

# ---------------------------------------------------------------- prepare ----

phase_prepare() {
  require_repo_root

  local new=${1:-}
  [ -n "$new" ] || die "no version given: release.sh prepare X.Y.Z"
  is_semver "$new" || die "'$new' is not X.Y.Z"

  if [ -n "$(git status --porcelain --untracked-files=no)" ]; then
    note "warning: working directory is not clean"
    git status --short --untracked-files=no
    confirm "Continue anyway?"
  fi

  local current
  current=$(project_version)
  [ -n "$current" ] || die "could not read the project version from $CMAKELISTS"
  echo -e "== current version: ${YELLOW}$current${NC}"
  echo -e "== target version:  ${YELLOW}$new${NC}"

  version_gt "$new" "$current" ||
    die "$new is not greater than the current version $current"

  local published
  published=$(pypi_latest || true)
  if [ -n "$published" ]; then
    echo "== latest on PyPI:  $published"
    version_gt "$new" "$published" ||
      die "$new is not greater than $published, already on PyPI. PyPI never accepts a version twice."
  else
    note "warning: could not reach PyPI; the release workflow's tag check is the backstop"
  fi

  sed -i -E "s/^([[:space:]]+VERSION[[:space:]]+)$current/\1$new/" "$CMAKELISTS"
  sed -i -E "s/(library_version\(\) == \")[^\"]*\"/\1$new\"/" "$VERSION_TEST"

  local major
  major=${new%%.*}
  if [ "$major" -eq 0 ]; then
    local compat
    compat=$(expected_compat "$new")
    sed -i -E "s/^([[:space:]]+COMPAT_VERSION[[:space:]]+)[0-9][0-9.]*/\1$compat/" "$CMAKELISTS"
  else
    note "warning: COMPAT_VERSION has no rule past 1.0; it is still $(compat_version). Set it by hand."
  fi

  [ "$(project_version)" = "$new" ] || die "failed to rewrite the version in $CMAKELISTS"
  [ "$(test_version)" = "$new" ] || die "failed to rewrite the version in $VERSION_TEST"

  if [ -n "$(changelog_section "$new")" ]; then
    note "$CHANGELOG already has a section for $new; leaving it"
  else
    local today skeleton
    today=$(date +%Y-%m-%d)
    skeleton=$(mktemp)
    cat >"$skeleton" <<EOF
## [$new] - $today

TODO: what this release is for

### Changed

### Added

### Fixed

EOF
    # Insert above the first real release heading, outside the template comment.
    awk -v skel="$skeleton" '
      incomment { print; if (/-->/) incomment = 0; next }
      /<!--/ { print; if (!/-->/) incomment = 1; next }
      !done && /^## \[/ { while ((getline line < skel) > 0) print line; done = 1 }
      { print }
      END { if (!done) { print ""; while ((getline line < skel) > 0) print line } }
    ' "$CHANGELOG" >"$CHANGELOG.new"
    mv "$CHANGELOG.new" "$CHANGELOG"
    rm -f "$skeleton"
  fi

  echo ""
  ok "prepared $new"
  echo ""
  echo "Next, write the release notes by hand:"
  echo ""
  echo -e "  1. Fill in ${YELLOW}## [$new]${NC} in $CHANGELOG. Anything a consumer of"
  echo "     $current sees differently goes first, under ### Changed. Delete empty subsections."
  echo "  2. Review the diff, then:"
  echo ""
  echo -e "     ${YELLOW}just release tag${NC}"
}

# -------------------------------------------------------------------- tag ----

phase_tag() {
  require_repo_root
  require_on_main
  require_up_to_date

  local version
  version=$(project_version)
  echo -e "== releasing version: ${YELLOW}$version${NC}"

  # 1. The version lines must agree.
  local tested compat
  tested=$(test_version)
  [ "$tested" = "$version" ] ||
    die "$VERSION_TEST expects '$tested', $CMAKELISTS says $version -- did 'prepare' run?"

  compat=$(compat_version)
  if [ "${version%%.*}" -eq 0 ]; then
    [ "$compat" = "$(expected_compat "$version")" ] ||
      die "COMPAT_VERSION is $compat; before 1.0 it must be $(expected_compat "$version")"
  elif version_gt "$compat" "$version"; then
    die "COMPAT_VERSION $compat is greater than VERSION $version"
  fi

  # 2. The tag must be new, here and on origin.
  local tag="v$version"
  tag_on_origin "$tag" && refuse_existing_tag "$tag" "on origin"
  git rev-parse --verify --quiet "refs/tags/$tag" >/dev/null && refuse_existing_tag "$tag" "locally"

  # 3. The notes must exist, be dated and be written.
  local section
  section=$(changelog_section "$version")
  [ -n "$section" ] ||
    die "no '## [$version]' section in $CHANGELOG. Write the release notes first."

  printf '%s\n' "$section" | head -1 | grep -qE "^## \[$version\] - [0-9]{4}-[0-9]{2}-[0-9]{2}$" ||
    die "the heading for $version must read '## [$version] - YYYY-MM-DD'"

  [ -n "$(changelog_body "$version" | tr -d '[:space:]')" ] ||
    die "the $CHANGELOG section for $version is empty."

  printf '%s\n' "$section" | grep -qiE 'TODO|FIXME|XXX|PLACEHOLDER' &&
    die "the $CHANGELOG section for $version still contains placeholder text."

  local dated
  dated=$(printf '%s\n' "$section" | head -1 | sed -nE 's/.* - ([0-9-]+)$/\1/p')
  [ "$dated" = "$(date +%Y-%m-%d)" ] ||
    note "note: the section is dated $dated, today is $(date +%Y-%m-%d)"

  echo ""
  echo "== release notes for $version:"
  printf '%s\n' "$section" | sed 's/^/   /'
  echo ""

  local others
  others=$(git status --porcelain --untracked-files=no -- . \
    ":!$CMAKELISTS" ":!$VERSION_TEST" ":!$CHANGELOG")
  if [ -n "$others" ]; then
    note "warning: these changes are NOT part of the release commit:"
    printf '%s\n' "$others"
  fi

  report_macos_floor
  echo ""

  confirm "Commit the release and create $tag?"

  if [ -n "$(git status --porcelain -- "$CMAKELISTS" "$VERSION_TEST" "$CHANGELOG")" ]; then
    git commit --quiet -m "chore: release $version" -- "$CMAKELISTS" "$VERSION_TEST" "$CHANGELOG"
    ok "committed chore: release $version"
  else
    note "nothing to commit, tagging the current HEAD"
  fi

  git tag -a "$tag" -m "arcana-tarot $version"

  ok "tag $tag created on $(git rev-parse --short HEAD)"
  echo ""
  echo "Next, push $BRANCH and the tag together:"
  echo ""
  echo -e "   ${YELLOW}just release push${NC}"
}

# ------------------------------------------------------------------- push ----

phase_push() {
  require_repo_root
  require_on_main
  require_up_to_date

  local version tag
  version=$(project_version)
  tag="v$version"

  git rev-parse --verify --quiet "refs/tags/$tag" >/dev/null ||
    die "no local tag $tag -- run 'just release tag' first."

  git merge-base --is-ancestor "$tag^{commit}" HEAD ||
    die "$tag is not an ancestor of $BRANCH -- it is left over from an earlier
   attempt. Delete it (git tag -d $tag) and re-tag."

  echo -e "== pushing ${YELLOW}$BRANCH${NC} and ${YELLOW}$tag${NC} to origin"

  git push --atomic origin "$BRANCH" "refs/tags/$tag" ||
    die "push refused -- nothing was pushed, including the tag.
   Rebase onto origin/$BRANCH and start over from 'prepare'."

  ok "pushed $BRANCH and $tag"
  echo ""
  echo "Optionally wait for CI on the release commit:"
  echo ""
  echo -e "   ${YELLOW}just release ci${NC}"
  echo ""
  echo "Then publish, which uploads to PyPI and cannot be undone:"
  echo ""
  echo -e "   ${YELLOW}just release publish${NC}"
}

# ---------------------------------------------------------------- publish ----

phase_publish() {
  require_repo_root
  command -v gh >/dev/null || die "gh is not installed"

  local version tag
  version=$(project_version)
  tag="v$version"

  tag_on_origin "$tag" ||
    die "$tag is not on origin -- run 'just release push' first."

  gh release view "$tag" >/dev/null 2>&1 &&
    die "a GitHub release for $tag already exists. It has been published."

  # The notes come from the tagged commit, and must match the working tree's,
  # so what is published is what was reviewed and tagged.
  tagged_tree=$(mktemp)
  notes=$(mktemp)
  trap 'rm -f "$tagged_tree" "$notes"' EXIT

  git show "$tag:$CHANGELOG" >"$tagged_tree" 2>/dev/null ||
    die "$tag has no $CHANGELOG"
  changelog_body "$version" "$tagged_tree" >"$notes"
  [ -s "$notes" ] || die "$tag's $CHANGELOG has no section for $version"

  [ "$(changelog_section "$version" "$tagged_tree")" = "$(changelog_section "$version")" ] ||
    die "the $CHANGELOG section for $version differs from the one in $tag.
   Notes changed after tagging are not published. Revert them, or cut a new version."

  local sha run
  sha=$(git rev-parse "$tag^{commit}")
  run=$(ci_run_for "$sha" '"\(.status) \(.conclusion) \(.url)"' || true)
  if [ -n "$run" ]; then
    echo "== CI on $(git rev-parse --short "$sha"): $run"
  else
    note "no CI run found for $(git rev-parse --short "$sha")"
  fi

  echo ""
  echo "== notes for arcana-tarot $version:"
  sed 's/^/   /' "$notes"
  echo ""

  confirm "Publish $tag? This uploads $DIST $version to PyPI, which can never be undone."

  gh release create "$tag" --verify-tag \
    --title "$DIST $version" \
    --notes-file "$notes"

  ok "released $tag:"
  echo ""
  echo "   gh run list --workflow release.yml --limit 1"
  echo "   gh run list --workflow macos-wheel.yml --limit 1"
}

# --------------------------------------------------------------------- ci ----

phase_ci() {
  command -v gh >/dev/null || die "gh is not installed"

  local sha id
  sha=$(git rev-parse HEAD)
  id=$(ci_run_for "$sha" '.databaseId' || true)
  [ -n "$id" ] ||
    die "no CI run for $(git rev-parse --short "$sha") yet. Has it been pushed? Try again in a moment."

  gh run watch "$id" --exit-status
}

# ---------------------------------------------------------------- dry-run ----

phase_dry_run() {
  command -v gh >/dev/null || die "gh is not installed"

  note "TestPyPI never accepts a version twice either, and the workflow skips"
  note "existing files. After the first dry run of $(project_version), this proves"
  note "the build, not the upload."

  local before
  before=$(gh run list --workflow release.yml --event workflow_dispatch --limit 1 \
    --json databaseId --jq '.[0].databaseId // 0')

  gh workflow run release.yml --ref "$BRANCH" -f publish=true

  local id="$before"
  for _ in $(seq 1 30); do
    id=$(gh run list --workflow release.yml --event workflow_dispatch --limit 1 \
      --json databaseId --jq '.[0].databaseId // 0')
    [ "$id" != "$before" ] && break
    sleep 2
  done
  [ "$id" != "$before" ] ||
    die "the dispatched run did not appear; see 'gh run list --workflow release.yml'"

  gh run watch "$id" --exit-status
}

# ------------------------------------------------------------------- main ----

case "${1:-}" in
prepare)
  shift
  phase_prepare "${1:-}"
  ;;
tag) phase_tag ;;
push) phase_push ;;
publish) phase_publish ;;
ci) phase_ci ;;
dry-run) phase_dry_run ;;
-h | --help | help | "")
  usage
  exit 1
  ;;
*)
  usage >&2
  die "unknown phase: $1"
  ;;
esac
