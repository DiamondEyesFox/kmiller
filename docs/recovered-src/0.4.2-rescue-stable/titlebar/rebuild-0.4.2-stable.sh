cd ~/Downloads/kmiller || exit 1

# Clean out the bad build
rm -rf build
mkdir build && cd build

# Configure with the stable tag
cmake -DKMILLER_VERSION=0.4.2-stable ..

# Compile
make -j"$(nproc)"

# Install to /opt
sudo make install

# Point the launcher to the new stable binary
sudo ln -sfn /opt/kmiller/versions/0.4.2-stable/bin/kmiller /usr/local/bin/kmiller
sudo sed -i 's|^Exec=.*|Exec=/opt/kmiller/versions/0.4.2-stable/bin/kmiller %U|' \
  /usr/local/share/applications/kmiller.desktop

# Refresh KDE menus
kbuildsycoca6 --noincremental 2>/dev/null || true
update-desktop-database /usr/local/share/applications 2>/dev/null || true

# Sanity check
readlink -f /usr/local/bin/kmiller
grep -m1 '^Exec=' /usr/local/share/applications/kmiller.desktop
