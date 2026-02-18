# Navigate to the right directory
cd /var/FBOSS/fboss

# Build using a cmake target
time ./fboss/oss/scripts/run-getdeps.py build --allow-system-packages \
  --extra-cmake-defines='{"CMAKE_BUILD_TYPE": "MinSizeRel", "CMAKE_CXX_STANDARD": "20", "RANGE_V3_TESTS": "OFF", "RANGE_V3_PERF": "OFF"}' \
  --scratch-path /var/FBOSS/tmp_bld_dir --cmake-target $TARGET fboss
