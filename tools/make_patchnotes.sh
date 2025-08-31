#!/usr/bin/env bash
set -euo pipefail
ROOT="${1:-.}"
VERSION="${2:-}"
cd "$ROOT"

if [[ -z "${VERSION}" ]]; then
  VERSION="$(awk 'tolower($0) ~ /project\\(kmiller/ {for(i=1;i<=NF;i++) if($i=="VERSION"){print $(i+1); exit}}' CMakeLists.txt || true)"
  : "${VERSION:=0.0.0}"
fi

NOTES_DIR="docs/patchnotes"
mkdir -p "$NOTES_DIR"
OUT="${NOTES_DIR}/${VERSION}.md"

# Try to get diff range
PREV_TAG="$(git tag --list | sort -V | grep -v "^v${VERSION}$" | tail -n1 || true)"
if [[ -n "$PREV_TAG" ]]; then
  RANGE="${PREV_TAG}..HEAD"
else
  FIRST="$(git rev-list --max-parents=0 HEAD | tail -n1)"
  RANGE="${FIRST}..HEAD"
fi

SUMMARY=$(git log --pretty=format:'- %s' ${RANGE} 2>/dev/null || true)
FILES_CHANGED=$(git diff --name-status ${RANGE} 2>/dev/null || true)
DIFF_RAW=$(git diff -U2 ${RANGE} 2>/dev/null || true)
DIFF_TRIM=$(echo "$DIFF_RAW" | awk 'NR<=1200{print} END{if (NR>1200) print "\n... (diff truncated) ..."}')

BUILD_CMDS=""
[[ -f build.log ]] && BUILD_CMDS="$(cat build.log)"

cat > "$OUT" <<MD
# KMiller ${VERSION} — Patch Notes
**Date:** $(date -u +'%Y-%m-%d %H:%M UTC')

## What changed (human-readable)
${SUMMARY:-(add a short summary here if empty)}

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
