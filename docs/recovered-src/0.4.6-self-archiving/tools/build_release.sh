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

# Fresh build dir
rm -rf "$BUILDDIR"
mkdir -p "$BUILDDIR"
cd "$BUILDDIR"

echo "[build] version: $VER" | tee "$LOG"
echo "[build] cmake configure" | tee -a "$LOG"
cmake -DKMILLER_VERSION="$VER" .. 2>&1 | tee -a "$LOG"

echo "[build] make" | tee -a "$LOG"
make -j"$(nproc)" 2>&1 | tee -a "$LOG"

echo "[build] install" | tee -a "$LOG"
sudo make install 2>&1 | tee -a "$LOG"

# Point launcher to this binary
sudo ln -sfn "/opt/kmiller/versions/$VER/bin/kmiller" /usr/local/bin/kmiller
echo "-- kmiller symlink -> /opt/kmiller/versions/$VER/bin/kmiller" | tee -a "$LOG"

# --- Package full source snapshot for this version ---
OUTDIR="/opt/kmiller/versions/$VER"
TARBALL="$OUTDIR/source-$VER.tar.gz"
echo "[build] packaging sources -> $TARBALL" | tee -a "$LOG"
( cd "$SRCDIR" && \
  tar czf "$TARBALL" src CMakeLists.txt tools 2>/dev/null || \
  tar czf "$TARBALL" src CMakeLists.txt 2>/dev/null )

echo "[build] done" | tee -a "$LOG"