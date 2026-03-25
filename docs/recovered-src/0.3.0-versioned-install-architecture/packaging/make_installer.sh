#!/usr/bin/env bash
set -euo pipefail
VER="${1:?Usage: $0 <version>}"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
PKG="$ROOT/packaging"
OUT="$ROOT/dist"
TODAY="$(date +%Y-%m-%d)"

# build kmiller as usual
rm -rf "$ROOT/build" && mkdir "$ROOT/build" && cd "$ROOT/build"
cmake -DCMAKE_BUILD_TYPE=Release -DKMILLER_VERSION="$VER" ..
make -j"$(nproc)"

# stage payload
STAGE="$PKG/packages/com.kmiller.core/data/kmiller-${VER}-linux-x86_64"
rm -rf "$STAGE"
mkdir -p "$STAGE/bin" "$STAGE/share/applications"
install -m755 "$ROOT/build/kmiller" "$STAGE/bin/kmiller"

cat > "$STAGE/share/applications/kmiller.desktop" <<EOF
[Desktop Entry]
Type=Application
Name=KMiller
GenericName=File Manager
Comment=Finder-style file manager with Miller Columns and Quick Look
Exec=kmiller %U
Icon=kmiller
Terminal=false
Categories=Qt;KDE;Utility;FileManager;
MimeType=inode/directory;
EOF

# fill in meta
sed -e "s/@APP_VERSION@/${VER}/g" -e "s/@TODAY@/${TODAY}/g" \
  -i "$PKG/packages/com.kmiller.core/meta/package.xml"

mkdir -p "$OUT"
binarycreator -c "$PKG/config/config.xml" -p "$PKG/packages" "$OUT/KMiller-${VER}-Installer"
echo "Installer ready: $OUT/KMiller-${VER}-Installer"
