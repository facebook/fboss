#!/bin/bash
# Navigate to the right directory
cd /var/FBOSS/fboss || exit

# Build using a cmake target
time ./build/fbcode_builder/getdeps.py build --allow-system-packages \
  --build-type MinSizeRel \
  --extra-cmake-defines='{"CMAKE_CXX_STANDARD": "20"}' \
  --scratch-path /var/FBOSS/tmp_bld_dir --src-dir . fboss
