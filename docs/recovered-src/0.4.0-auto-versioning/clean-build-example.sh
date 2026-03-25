# clean build
rm -rf build && mkdir build && cd build
cmake -DKMILLER_VERSION=0.4.0 ..   # or tag your git repo and omit this
make -j"$(nproc)"
sudo make install
