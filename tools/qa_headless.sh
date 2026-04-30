#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BIN="${1:-$ROOT_DIR/build/kmiller}"

if [[ ! -x "$BIN" ]]; then
  echo "QA failed: binary not executable: $BIN" >&2
  exit 1
fi

TMP_ROOT="$(mktemp -d "${TMPDIR:-/tmp}/kmiller-qa.XXXXXX")"
trap 'rm -rf "$TMP_ROOT"' EXIT

QA_HOME="$TMP_ROOT/home"
QA_CONFIG="$TMP_ROOT/config"
QA_CACHE="$TMP_ROOT/cache"
QA_DATA="$TMP_ROOT/data"
QA_RUNTIME="$TMP_ROOT/runtime"
QA_DIR="$TMP_ROOT/files"
mkdir -p "$QA_HOME" "$QA_CONFIG" "$QA_CACHE" "$QA_DATA" "$QA_RUNTIME" "$QA_DIR/subdir"
chmod 700 "$QA_RUNTIME"

printf 'hello\n' > "$QA_DIR/alpha.txt"
printf 'world\n' > "$QA_DIR/subdir/beta.txt"
ln -s "$QA_DIR/subdir" "$QA_DIR/link-to-subdir"

echo "QA fixture: $QA_DIR"

dbus-run-session -- \
  xvfb-run -a \
  env \
    HOME="$QA_HOME" \
    XDG_CONFIG_HOME="$QA_CONFIG" \
    XDG_CACHE_HOME="$QA_CACHE" \
    XDG_DATA_HOME="$QA_DATA" \
    XDG_DATA_DIRS="${XDG_DATA_DIRS:-/usr/local/share:/usr/share}" \
    XDG_RUNTIME_DIR="$QA_RUNTIME" \
    QT_QPA_PLATFORM=xcb \
    "$BIN" --qa-fixture "$QA_DIR" \
  >/tmp/kmiller-qa-ops.stdout 2>/tmp/kmiller-qa-ops.stderr || {
    echo "QA failed: fixture operations failed" >&2
    echo "--- stdout ---" >&2
    cat /tmp/kmiller-qa-ops.stdout >&2 || true
    echo "--- stderr ---" >&2
    cat /tmp/kmiller-qa-ops.stderr >&2 || true
    exit 1
  }

dbus-run-session -- \
  xvfb-run -a \
  env \
    HOME="$QA_HOME" \
    XDG_CONFIG_HOME="$QA_CONFIG" \
    XDG_CACHE_HOME="$QA_CACHE" \
    XDG_DATA_HOME="$QA_DATA" \
    XDG_DATA_DIRS="${XDG_DATA_DIRS:-/usr/local/share:/usr/share}" \
    XDG_RUNTIME_DIR="$QA_RUNTIME" \
    QT_QPA_PLATFORM=xcb \
    "$BIN" --qa-archive "$QA_DIR" \
  >/tmp/kmiller-qa-archive.stdout 2>/tmp/kmiller-qa-archive.stderr || {
    echo "QA failed: archive checks failed" >&2
    echo "--- stdout ---" >&2
    cat /tmp/kmiller-qa-archive.stdout >&2 || true
    echo "--- stderr ---" >&2
    cat /tmp/kmiller-qa-archive.stderr >&2 || true
    exit 1
  }

dbus-run-session -- \
  xvfb-run -a \
  env \
    HOME="$QA_HOME" \
    XDG_CONFIG_HOME="$QA_CONFIG" \
    XDG_CACHE_HOME="$QA_CACHE" \
    XDG_DATA_HOME="$QA_DATA" \
    XDG_DATA_DIRS="${XDG_DATA_DIRS:-/usr/local/share:/usr/share}" \
    XDG_RUNTIME_DIR="$QA_RUNTIME" \
    QT_QPA_PLATFORM=xcb \
    "$BIN" --qa-open-with "$QA_DIR/alpha.txt" \
  >/tmp/kmiller-qa-open-with.stdout 2>/tmp/kmiller-qa-open-with.stderr || {
    echo "QA failed: Open With discovery checks failed" >&2
    echo "--- stdout ---" >&2
    cat /tmp/kmiller-qa-open-with.stdout >&2 || true
    echo "--- stderr ---" >&2
    cat /tmp/kmiller-qa-open-with.stderr >&2 || true
    exit 1
  }

dbus-run-session -- \
  xvfb-run -a \
  env \
    HOME="$QA_HOME" \
    XDG_CONFIG_HOME="$QA_CONFIG" \
    XDG_CACHE_HOME="$QA_CACHE" \
    XDG_DATA_HOME="$QA_DATA" \
    XDG_DATA_DIRS="${XDG_DATA_DIRS:-/usr/local/share:/usr/share}" \
    XDG_RUNTIME_DIR="$QA_RUNTIME" \
    QT_QPA_PLATFORM=xcb \
    "$BIN" --qa-ui-logic "$QA_DIR" \
  >/tmp/kmiller-qa-ui.stdout 2>/tmp/kmiller-qa-ui.stderr || {
    echo "QA failed: UI logic checks failed" >&2
    echo "--- stdout ---" >&2
    cat /tmp/kmiller-qa-ui.stdout >&2 || true
    echo "--- stderr ---" >&2
    cat /tmp/kmiller-qa-ui.stderr >&2 || true
    exit 1
  }

dbus-run-session -- \
  xvfb-run -a \
  env \
    HOME="$QA_HOME" \
    XDG_CONFIG_HOME="$QA_CONFIG" \
    XDG_CACHE_HOME="$QA_CACHE" \
    XDG_DATA_HOME="$QA_DATA" \
    XDG_DATA_DIRS="${XDG_DATA_DIRS:-/usr/local/share:/usr/share}" \
    XDG_RUNTIME_DIR="$QA_RUNTIME" \
    QT_QPA_PLATFORM=xcb \
    timeout 5s "$BIN" --path "$QA_DIR" --app-id kmiller-qa \
  >/tmp/kmiller-qa.stdout 2>/tmp/kmiller-qa.stderr || status=$?

status="${status:-0}"
if [[ "$status" != "0" && "$status" != "124" ]]; then
  echo "QA failed: KMiller exited with status $status" >&2
  echo "--- stdout ---" >&2
  cat /tmp/kmiller-qa.stdout >&2 || true
  echo "--- stderr ---" >&2
  cat /tmp/kmiller-qa.stderr >&2 || true
  exit "$status"
fi

if grep -Eiq 'segmentation fault|assert|abort|failed to create|could not load|not found' /tmp/kmiller-qa.stderr; then
  echo "QA failed: suspicious stderr output" >&2
  cat /tmp/kmiller-qa.stderr >&2
  exit 1
fi

echo "Headless QA OK: launched at fixture path and stayed alive for smoke window."
