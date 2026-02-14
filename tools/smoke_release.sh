#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${1:-/tmp/kmiller-smoke-build}"

cmake -S "$ROOT_DIR" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release
cmake --build "$BUILD_DIR" -j"$(nproc)"

BIN="$BUILD_DIR/kmiller"
if [ ! -x "$BIN" ]; then
  echo "Smoke failed: binary not found at $BIN" >&2
  exit 1
fi

if ldd "$BIN" | grep -q 'not found'; then
  echo "Smoke failed: unresolved shared libraries" >&2
  ldd "$BIN" | grep 'not found' >&2
  exit 1
fi

echo "Smoke OK: build succeeded and no missing shared libraries"
