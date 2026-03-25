#!/usr/bin/env bash
set -euo pipefail

BASE="/opt/kmiller"
VERS_DIR="$BASE/versions"
BIN_LINK="/usr/local/bin/kmiller"
KEEP_N="${KEEP_N:-5}"   # keep last N versions

require_root() { if [[ $EUID -ne 0 ]]; then echo "Run with sudo"; exit 1; fi; }

usage() {
  cat <<EOF
KMiller updater
Usage:
  sudo kmiller-update build        # install from current build dir (./build/kmiller)
  sudo kmiller-update install <KMILLER_VERSION> <path-to-kmiller>  # manual
  sudo kmiller-update list
  sudo kmiller-update switch <KMILLER_VERSION>
  sudo kmiller-update prune [N]    # keep last N (default $KEEP_N)
EOF
}

install_version() {
  local ver="$1"
  local src_bin="$2"
  local dst="$VERS_DIR/$ver/bin"
  mkdir -p "$dst"
  install -m755 "$src_bin" "$dst/kmiller"
  ln -sfn "$dst/kmiller" "$BIN_LINK"
  echo "Installed $ver. Current -> $(readlink -f $BIN_LINK)"
}

cmd_build() {
  require_root
  local ver
  ver="$(grep -Po '(?<=project\\(KMiller VERSION )[^ )]+' CMakeLists.txt || echo unknown)"
  [[ -x build/kmiller ]] || { echo "build/kmiller not found"; exit 1; }
  install_version "$ver" "build/kmiller"
}

cmd_install() { require_root; install_version "$1" "$2"; }
cmd_list() {
  echo "Installed versions:"
  ls -1 "$VERS_DIR" 2>/dev/null | sort -V || true
  echo "Current -> $(readlink -f $BIN_LINK 2>/dev/null || echo none)"
}
cmd_switch() {
  require_root
  local ver="$1"; local exe="$VERS_DIR/$ver/bin/kmiller"
  [[ -x "$exe" ]] || { echo "Version $ver not found"; exit 1; }
  ln -sfn "$exe" "$BIN_LINK"; echo "Switched to $ver"
}
cmd_prune() {
  require_root
  local keep="${1:-$KEEP_N}"
  mapfile -t vers < <(ls -1 "$VERS_DIR" 2>/dev/null | sort -V) || vers=()
  local del_count=$(( ${#vers[@]} - keep ))
  if (( del_count > 0 )); then
    for v in "${vers[@]:0:del_count}"; do rm -rf "$VERS_DIR/$v"; echo "Removed $v"; done
  fi
}

case "${1:-}" in
  build)   cmd_build ;;
  install) shift; cmd_install "$@" ;;
  list)    cmd_list ;;
  switch)  shift; cmd_switch "$@" ;;
  prune)   shift; cmd_prune "${1:-}";;
  *)       usage; exit 1;;
esac
