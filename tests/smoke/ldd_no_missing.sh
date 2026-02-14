#!/usr/bin/env bash
set -euo pipefail

BIN="${1:?Usage: ldd_no_missing.sh /path/to/kmiller}"

if [ ! -x "$BIN" ]; then
  echo "Expected executable binary, got: $BIN" >&2
  exit 1
fi

MISSING="$(ldd "$BIN" | grep 'not found' || true)"
if [ -n "$MISSING" ]; then
  echo "Missing shared libraries detected:" >&2
  echo "$MISSING" >&2
  exit 1
fi

echo "ldd smoke passed for $BIN"
