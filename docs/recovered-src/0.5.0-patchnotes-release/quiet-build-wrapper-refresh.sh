set -euo pipefail
ROOT=~/Downloads/kmiller
mkdir -p "$ROOT/tools" "$ROOT/docs/patchnotes"
cd "$ROOT"

# make_patchnotes.sh
cat > tools/make_patchnotes.sh <<'EOF'
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
EOF
chmod +x tools/make_patchnotes.sh

# build_release.sh (quiet logging; no tee; -j2 to be gentle)
cat > tools/build_release.sh <<'EOF'
#!/usr/bin/env bash
set -euo pipefail
ROOT="${1:-$PWD}"
VERSION="${2:-}"
cd "$ROOT"

if [[ -z "${VERSION}" ]]; then
  echo "Usage: tools/build_release.sh <project-root> <version>"; exit 1
fi

# Log only to file (quiet terminal)
exec >build.log 2>&1

echo "[build] version: ${VERSION}"
rm -rf build
mkdir -p build
cd build
echo "[build] cmake configure"
cmake -DKMILLER_VERSION="${VERSION}" ..
echo "[build] make -j2"
make -j2
echo "[build] sudo make install"
sudo make install
cd "$ROOT"

# Generate patch notes
./tools/make_patchnotes.sh "$ROOT" "${VERSION}"

# Commit notes and tag version (best effort)
git add docs/patchnotes/"${VERSION}.md" || true
git commit -m "notes: ${VERSION}" || true
git tag -f "v${VERSION}" || true

# Reinstall to ensure notes copied/symlinked
cd build
sudo make install
echo "[done] Installed KMiller ${VERSION} with patch notes."
EOF
chmod +x tools/build_release.sh

# Add Help -> Patch Notes… to menu if not present
if ! grep -q "Patch Notes…" src/MainWindow.cpp 2>/dev/null; then
  awk '
  /menuBar\(\)/ && /addMenu/ && !p {
    print;
    print "    auto *helpMenu = menuBar()->addMenu(\"&Help\");";
    print "    QAction *actNotes = helpMenu->addAction(\"Patch Notes…\");";
    print "    connect(actNotes, &QAction::triggered, this, []{";
    print "        QUrl u = QUrl::fromLocalFile(\"/opt/kmiller/PATCHNOTES-latest.md\");";
    print "        QDesktopServices::openUrl(u);";
    print "    });";
    p=1; next
  }1' src/MainWindow.cpp > /tmp/MainWindow.cpp.new && mv /tmp/MainWindow.cpp.new src/MainWindow.cpp
  sed -i '1i #include <QDesktopServices>\n#include <QUrl>' src/MainWindow.cpp
fi

echo "✔ tools created at $ROOT/tools"
