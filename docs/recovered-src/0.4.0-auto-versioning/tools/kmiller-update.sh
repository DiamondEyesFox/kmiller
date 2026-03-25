#!/usr/bin/env bash
set -euo pipefail
BASE="/opt/kmiller"; VERS="$BASE/versions"; BIN="/usr/local/bin/kmiller"; KEEP_N="${KEEP_N:-5}"
require_root(){ [[ $EUID -eq 0 ]] || { echo "Run with sudo"; exit 1; }; }
ver_from_cmake(){ grep -Po '(?<=project\(KMiller VERSION )[^ )]+' CMakeLists.txt || echo unknown; }
install_ver(){ local v="$1" b="$2"; local d="$VERS/$v/bin"; mkdir -p "$d"; install -m755 "$b" "$d/kmiller"; ln -sfn "$d/kmiller" "$BIN"; echo "Installed $v -> $(readlink -f "$BIN")"; }
list(){ echo "Versions:"; ls -1 "$VERS" 2>/dev/null | sort -V || true; echo "Current -> $(readlink -f "$BIN" 2>/dev/null || echo none)"; }
switch(){ local v="$1"; [[ -x "$VERS/$v/bin/kmiller" ]] || { echo "No such $v"; exit 1; }; ln -sfn "$VERS/$v/bin/kmiller" "$BIN"; echo "Switched to $v"; }
prune(){ local k="${1:-$KEEP_N}"; mapfile -t arr < <(ls -1 "$VERS" 2>/dev/null | sort -V); local del=$(( ${#arr[@]}-k )); (( del>0 )) && for v in "${arr[@]:0:del}"; do rm -rf "$VERS/$v"; echo "Removed $v"; done; }
case "${1:-}" in
  build) require_root; v="$(ver_from_cmake)"; [[ -x build/kmiller ]] || { echo "build/kmiller missing"; exit 1; }; install_ver "$v" build/kmiller;;
  list) list;;
  switch) require_root; switch "${2:?version}";;
  prune) require_root; prune "${2:-}";;
  install) require_root; install_ver "${2:?version}" "${3:?path-to-kmiller}";;
  *) echo "Usage: sudo kmiller-update {build|install VER PATH|list|switch VER|prune [N]}"; exit 1;;
esac