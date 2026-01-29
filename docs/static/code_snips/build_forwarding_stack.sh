#!/bin/bash
# Navigate to the right directory
cd /var/FBOSS/fboss || exit

# Set environment variables appropriate for your build
export SAI_BRCM_IMPL=1

# Start the build
time ./build/fbcode_builder/getdeps.py build --allow-system-packages \
  --build-type MinSizeRel \
  --extra-cmake-defines='{"CMAKE_CXX_STANDARD": "20"}' \
  --scratch-path /var/FBOSS/tmp_bld_dir fboss
