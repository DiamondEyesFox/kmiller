  rm -rf build && mkdir build && cd build
  cmake -DKMILLER_VERSION=0.4.1 ..
  make -j"$(nproc)"
  sudo make install
  