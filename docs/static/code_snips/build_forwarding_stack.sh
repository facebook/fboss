# Navigate to the right directory
cd /var/FBOSS/fboss

# Set environment variables appropriate for your build
export SAI_BRCM_IMPL=1

# Start the build
time ./fboss/oss/scripts/run-getdeps.py \
  --npu-sai-version 1.16.1 \
  --npu-libsai-impl-path /opt/sdk/libsai_impl.a \
  --npu-experiments-path /opt/sdk/experimental \
  build \
  --allow-system-packages \
  --extra-cmake-defines='{"CMAKE_BUILD_TYPE": "MinSizeRel", "CMAKE_CXX_STANDARD": "20", "RANGE_V3_TESTS": "OFF", "RANGE_V3_PERF": "OFF"}' \
  --scratch-path /var/FBOSS/tmp_bld_dir \
  fboss
