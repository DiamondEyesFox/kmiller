# ==== paths ====
SRC=~/Downloads/kmiller
VER=0.4.2
INSTALL_DIR=/opt/kmiller/versions/$VER
BACKUP_DIR=~/kmiller-backups/$VER
mkdir -p "$BACKUP_DIR"

# 1) Source snapshot (pre-git safety net)
tar czf "$BACKUP_DIR/kmiller-src-$VER.tgz" -C "$SRC" .

# 2) Initialize Git (local repo) and commit everything
cd "$SRC"
# basic identity just for this repo; remove if you already set global config
git config user.name  "KMiller Local"
git config user.email "local@example.invalid" || true

git init
printf '%s\n' \
  'build/' \
  '*.o' '*.obj' \
  '*.user' '*.pro.user' \
  '.DS_Store' \
  '.vscode/' '.idea/' \
  > .gitignore

git add -A
git commit -m "KMiller $VER (stable snapshot)"

# Tag the stable build
git tag "v$VER"

# 3) Create a **bare mirror** backup (best for full restoration)
mkdir -p ~/kmiller-mirrors
git clone --mirror "$SRC" ~/kmiller-mirrors/kmiller.git

# 4) Back up the installed binary/versioned tree
if [ -d "$INSTALL_DIR" ]; then
  sudo tar czf "$BACKUP_DIR/kmiller-install-$VER.tgz" -C /opt/kmiller/versions "$VER"
  # make a user-owned copy for easy browsing (optional)
  mkdir -p "$BACKUP_DIR/install-copy"
  sudo cp -a "$INSTALL_DIR" "$BACKUP_DIR/install-copy/" && sudo chown -R "$USER":"$USER" "$BACKUP_DIR/install-copy"
fi

# 5) Checksums for integrity
cd "$BACKUP_DIR"
sha256sum kmiller-src-$VER.tgz  > SHA256SUMS.txt
[ -f kmiller-install-$VER.tgz ] && sha256sum kmiller-install-$VER.tgz >> SHA256SUMS.txt

# 6) What you now have + quick restore tips
echo
echo "== Backups written to: $BACKUP_DIR =="
ls -lh "$BACKUP_DIR"
echo
echo "Restore from **bare mirror**:"
echo "  git clone ~/kmiller-mirrors/kmiller.git kmiller-restore"
echo "  cd kmiller-restore && git checkout v$VER"
echo
echo "Restore from **source tarball**:"
echo "  mkdir kmiller-restore && tar xzf $BACKUP_DIR/kmiller-src-$VER.tgz -C kmiller-restore"
echo
echo "Restore installed build to /opt (if needed):"
echo "  sudo tar xzf $BACKUP_DIR/kmiller-install-$VER.tgz -C /opt/kmiller/versions"
echo
