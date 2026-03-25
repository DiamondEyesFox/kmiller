# build a new version
cd ~/Downloads/kmiller
sed -i 's/^project(KMiller VERSION .*)/project(KMiller VERSION 0.4.0)/' CMakeLists.txt  # bump
rm -rf build && mkdir build && cd build
cmake ..
make -j"$(nproc)"

# install as the new “current” and archive old ones
sudo make install
sudo kmiller-update build      # rotates symlink to 0.4.0, old stays in /opt/kmiller/versions
sudo kmiller-update list
# optional:
sudo kmiller-update prune 5    # keep last 5 versions
