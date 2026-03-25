#!/usr/bin/env bash
set -euo pipefail
ROOT="${1:-.}"
VERSION="${2:-}"
cd "$ROOT"

if [[ -z "${VERSION}" ]]; then
  # Try to parse from CMakeLists (project(KMiller VERSION x.y.z))
  VERSION="$(awk 'tolower($0) ~ /project\\(kmiller/ {for(i=1;i<=NF;i++) if($i=="VERSION"){print $(i+1); exit}}' CMakeLists.txt)"
  : "${VERSION:=0.0.0}"
fi

NOTES_DIR="docs/patchnotes"
mkdir -p "$NOTES_DIR"
OUT="${NOTES_DIR}/${VERSION}.md"

# figure previous tag to diff against (best effort)
PREV_TAG="$(git tag --list | sort -V | tail -n +1 | grep -v "^v${VERSION}$" | tail -n1 || true)"
RANGE=""
if [[ -n "$PREV_TAG" ]]; then
  RANGE="${PREV_TAG}..HEAD"
else
  # first release or no tags — use initial commit
  FIRST="$(git rev-list --max-parents=0 HEAD | tail -n1)"
  RANGE="${FIRST}..HEAD"
fi

# Natural language summary from commits (compact)
SUMMARY=$(git log --pretty=format:'- %s' ${RANGE} || true)

# Files changed (concise)
FILES_CHANGED=$(git diff --name-status ${RANGE} || true)

# Code diff (bounded)
# Keep diffs readable: context=2, max lines to 1200
DIFF_RAW=$(git diff -U2 ${RANGE} || true)
DIFF_TRIM=$(echo "$DIFF_RAW" | awk 'NR<=1200{print} END{if (NR>1200) print "\n... (diff truncated) ..."}')

# Build commands used (from last build/install log)
BUILD_CMDS=""
if [[ -f build.log ]]; then BUILD_CMDS="$(cat build.log)"; fi

cat > "$OUT" <<MD
# KMiller ${VERSION} — Patch Notes
**Date:** $(date -u +'%Y-%m-%d %H:%M UTC')

## What changed (human-readable)
${SUMMARY:-(no commit messages found — add a short summary here)}

## Files changed
\`\`\`
${FILES_CHANGED:-(no file changes detected)}
\`\`\`

## Code diff (unified)
\`\`\`diff
${DIFF_TRIM:-(no diff)}
\`\`\`

## Commands invoked for this build
\`\`\`
${BUILD_CMDS:-(no build.log found)}
\`\`\`

MD

echo "Wrote ${OUT}"