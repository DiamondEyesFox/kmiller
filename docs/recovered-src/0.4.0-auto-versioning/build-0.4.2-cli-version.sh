rm -rf build && mkdir build && cd build
cmake -DKMILLER_VERSION=0.4.2 ..
make -j"$(nproc)"
sudo kmiller-update build
sudo kmiller-update list
