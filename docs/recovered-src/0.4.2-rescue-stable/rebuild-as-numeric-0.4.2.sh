# rebuild as a plain numeric version
cd ~/Downloads/kmiller || exit 1
rm -rf build && mkdir build && cd build
cmake -DKMILLER_VERSION=0.4.2 ..
make -j"$(nproc)"
sudo make install

# create a 'rescue' alias without touching CMake:
sudo ln -sfn /opt/kmiller/versions/0.4.2 /opt/kmiller/versions/0.4.2-rescue

# point CLI + desktop to the rescue alias (so we can swap targets later if needed)
sudo ln -sfn /opt/kmiller/versions/0.4.2-rescue/bin/kmiller /usr/local/bin/kmiller
sudo sed -i 's|^Exec=.*|Exec=/opt/kmiller/versions/0.4.2-rescue/bin/kmiller %U|' /usr/local/share/applications/kmiller.desktop
kbuildsycoca6 --noincremental 2>/dev/null || true
update-desktop-database /usr/local/share/applications 2>/dev/null || true

# sanity checks
echo "CLI -> $(readlink -f /usr/local/bin/kmiller)"
echo "Desktop Exec -> $(grep -m1 '^Exec=' /usr/local/share/applications/kmiller.desktop)"
ls -l /opt/kmiller/versions/0.4.2/bin/kmiller
ls -ld /opt/kmiller/versions/0.4.2-rescue
