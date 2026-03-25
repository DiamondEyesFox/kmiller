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