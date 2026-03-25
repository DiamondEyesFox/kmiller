set -euo pipefail

ROOT=~/Downloads/kmiller
cd "$ROOT"

mkdir -p tools docs/patchnotes

# 0) Ensure git repo exists (so we can diff/track)
if ! git rev-parse --is-inside-work-tree >/dev/null 2>&1; then
  git init
  git add -A
  git commit -m "KMiller: initial import"
fi

# 1) Patch notes generator: natural language + file list + code diffs + build commands
cat > tools/make_patchnotes.sh <<'EOF'
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
EOF
chmod +x tools/make_patchnotes.sh

# 2) Build wrapper: logs commands, builds, installs, generates notes, tags
cat > tools/build_release.sh <<'EOF'
#!/usr/bin/env bash
set -euo pipefail
ROOT="${1:-$PWD}"
VERSION="${2:-}"
cd "$ROOT"

if [[ -z "${VERSION}" ]]; then
  echo "Usage: tools/build_release.sh <project-root> <version>"; exit 1
fi

# Log every command we run
exec > >(tee build.log) 2>&1

echo "[build] version: ${VERSION}"
echo "[build] configuring..."
rm -rf build
mkdir -p build
cd build
cmake -DKMILLER_VERSION="${VERSION}" ..

echo "[build] compiling..."
make -j"$(nproc)"

echo "[build] installing..."
sudo make install

cd "$ROOT"

# Generate patch notes (uses build.log)
./tools/make_patchnotes.sh "$ROOT" "${VERSION}"

# Commit notes and tag version
git add docs/patchnotes/"${VERSION}.md"
git commit -m "notes: ${VERSION}"
git tag -f "v${VERSION}"

# Reinstall to copy notes into /opt/kmiller/versions/<ver> and archive/symlink
cd build
sudo make install

echo "[done] Installed KMiller ${VERSION} with patch notes."
EOF
chmod +x tools/build_release.sh

# 3) Add a Help -> Patch Notes… menu item (opens latest)
#    This inserts a small action into MainWindow.cpp; idempotent.
if ! grep -q "Patch Notes…" src/MainWindow.cpp; then
  awk '
  /addMenu\("Help"\)/ && !p {
     print;
     print "    // Help menu: Patch Notes";
     print "    auto *helpMenu = menuBar()->addMenu(\"&Help\");";
     print "    QAction *actNotes = helpMenu->addAction(\"Patch Notes…\");";
     print "    connect(actNotes, &QAction::triggered, this, []{";
     print "        QUrl u = QUrl::fromLocalFile(\"/opt/kmiller/PATCHNOTES-latest.md\");";
     print "        QDesktopServices::openUrl(u);";
     print "    });";
     p=1; next
  }1' src/MainWindow.cpp > /tmp/MainWindow.cpp.new && mv /tmp/MainWindow.cpp.new src/MainWindow.cpp
fi

# Ensure needed include for QDesktopServices
if ! grep -q "QDesktopServices" src/MainWindow.cpp; then
  sed -i '1i #include <QDesktopServices>' src/MainWindow.cpp
  sed -i '1i #include <QUrl>' src/MainWindow.cpp
fi

# 4) Build once to make sure it compiles after menu tweak
mkdir -p build && cd build
cmake ..
make -j"$(nproc)"

echo
echo "✔ Tools ready."
echo
echo "Release with patch notes by running:"
echo "  ${ROOT}/tools/build_release.sh ${ROOT} 0.5.0"
echo
echo "Then start KMiller as usual: kmiller"
