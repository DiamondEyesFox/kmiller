#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 2 ]]; then
  echo "Usage: $0 <srcdir> <version>"
  exit 1
fi

SRCDIR="$(cd "$1" && pwd)"
VER="$2"
BUILDDIR="$SRCDIR/build"
LOG="$SRCDIR/build.log"

rm -rf "$BUILDDIR"
mkdir -p "$BUILDDIR"
cd "$BUILDDIR"

echo "[build] version: $VER" | tee "$LOG"
cmake -DKMILLER_VERSION="$VER" .. 2>&1 | tee -a "$LOG"
make -j"$(nproc)" 2>&1 | tee -a "$LOG"
sudo make install 2>&1 | tee -a "$LOG"

# update launcher symlink
sudo ln -sfn "/opt/kmiller/versions/$VER/bin/kmiller" /usr/local/bin/kmiller

# package source snapshot
OUTDIR="/opt/kmiller/versions/$VER"
TARBALL="$OUTDIR/source-$VER.tar.gz"
( cd "$SRCDIR" && tar czf "$TARBALL" src CMakeLists.txt tools )

echo "[build] done -> $OUTDIR"
