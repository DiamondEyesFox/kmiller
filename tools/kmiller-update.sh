#!/usr/bin/env bash
set -euo pipefail

BASE="/opt/kmiller"
VERS_DIR="$BASE/versions"
BIN_LINK="/usr/local/bin/kmiller"
KEEP_N="${KEEP_N:-5}"

red(){ printf "\033[1;31m%s\033[0m\n" "$*" >&2; }
green(){ printf "\033[1;32m%s\033[0m\n" "$*"; }
note(){ printf "\033[1;36m%s\033[0m\n" "$*"; }

need_root() { [[ $EUID -eq 0 ]] || { red "Run with sudo"; exit 1; }; }

ver_from_cmake() {
  # try build cache first, then CMakeLists
  if [[ -f build/CMakeCache.txt ]]; then
    grep -Po '(?<=KMILLER_VERSION:STRING=).*' build/CMakeCache.txt && return 0
  fi
  grep -Po '(?<=project\(\s*KMiller\s+VERSION\s)[^ )]+' CMakeLists.txt 2>/dev/null || echo "unknown"
}

install_version() {
  local ver="$1"; local src="$2"
  [[ -x "$src" ]] || { red "Binary not executable: $src"; exit 1; }
  local dst="$VERS_DIR/$ver/bin"
  mkdir -p "$dst"
  install -m755 "$src" "$dst/kmiller"
  ln -sfn "$dst/kmiller" "$BIN_LINK"
  green "Installed KMiller $ver"
  note  "Current -> $(readlink -f "$BIN_LINK")"
}

cmd_build() {
  need_root
  [[ -x build/kmiller ]] || { red "build/kmiller not found; build first"; exit 1; }
  local ver; ver="$(ver_from_cmake)"
  install_version "$ver" "build/kmiller"
}

cmd_install() { need_root; install_version "${1:?version}" "${2:?path-to-kmiller}"; }

cmd_list() {
  note "Installed versions:"
  ls -1 "$VERS_DIR" 2>/dev/null | sort -V || true
  echo
  note "Current -> $(readlink -f "$BIN_LINK" 2>/dev/null || echo none)"
}

cmd_switch() {
  need_root
  local ver="${1:?version}"
  local exe="$VERS_DIR/$ver/bin/kmiller"
  [[ -x "$exe" ]] || { red "Version $ver not found at $exe"; exit 1; }
  ln -sfn "$exe" "$BIN_LINK"
  green "Switched to $ver"
}

cmd_remove() {
  need_root
  local ver="${1:?version}"
  [[ -d "$VERS_DIR/$ver" ]] || { red "No such version: $ver"; exit 1; }
  # refuse to remove the one currently linked
  if [[ "$(readlink -f "$BIN_LINK")" == "$(readlink -f "$VERS_DIR/$ver/bin/kmiller")" ]]; then
    red "Refusing to remove the CURRENT version ($ver). Switch first."
    exit 1
  fi
  rm -rf "$VERS_DIR/$ver"
  green "Removed $ver"
}

cmd_prune() {
  need_root
  local keep="${1:-$KEEP_N}"
  mapfile -t v < <(ls -1 "$VERS_DIR" 2>/dev/null | sort -V)
  (( ${#v[@]} <= keep )) && { note "Nothing to prune (have ${#v[@]}, keep $keep)"; exit 0; }
  local del=$(( ${#v[@]} - keep ))
  for ver in "${v[@]:0:del}"; do
    # skip current
    if [[ "$(readlink -f "$BIN_LINK")" == "$(readlink -f "$VERS_DIR/$ver/bin/kmiller")" ]]; then
      note "Skipping current version $ver"
      continue
    fi
    rm -rf "$VERS_DIR/$ver"
    green "Pruned $ver"
  done
}

cmd_current() {
  readlink -f "$BIN_LINK" 2>/dev/null || { red "No current symlink"; exit 1; }
}

usage() {
cat <<EOF
KMiller updater

Usage:
  sudo kmiller-update build                    Install from ./build/kmiller (uses CMake version)
  sudo kmiller-update install <VER> <BIN>      Install specific binary as version VER
  kmiller-update list                          List installed versions + current
  sudo kmiller-update switch <VER>             Point launcher to version VER
  sudo kmiller-update remove <VER>             Remove version VER (not the current)
  sudo kmiller-update prune [N]                Keep only newest N (default $KEEP_N)
  kmiller-update current                       Show the current resolved binary

ENV:
  KEEP_N=5  # default number of versions to keep for 'prune'
EOF
}

case "${1:-}" in
  build)   shift; cmd_build "$@" ;;
  install) shift; cmd_install "$@" ;;
  list)    shift; cmd_list "$@" ;;
  switch)  shift; cmd_switch "$@" ;;
  remove)  shift; cmd_remove "$@" ;;
  prune)   shift; cmd_prune "${1:-}" ;;
  current) shift; cmd_current "$@" ;;
  *) usage; [[ -n "${1:-}" ]] && exit 1;;
esac
