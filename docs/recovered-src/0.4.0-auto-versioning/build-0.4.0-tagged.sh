  git tag -a v0.4.0 -m "KMiller 0.4.0"
  rm -rf build && mkdir build && cd build
  cmake ..
  make -j"$(nproc)"
  sudo make install
  