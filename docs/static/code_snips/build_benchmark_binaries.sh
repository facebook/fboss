#!/bin/bash
# Navigate to the right directory
cd /var/FBOSS/fboss || exit

# Start the build with --benchmark-install to include benchmark binaries
# NOTE: Choose the appropriate --npu-sai-impl and --npu-sai-sdk-version values
# for your platform. Run ./fboss/oss/scripts/run-getdeps.py -h to see
# Meta officially supported values.
time ./fboss/oss/scripts/run-getdeps.py \
  --npu-sai-impl SAI_BRCM_IMPL \
  --npu-sai-sdk-version SAI_VERSION_14_0_EA_ODP \
  --npu-sai-version 1.16.1 \
  --npu-libsai-impl-path /opt/sdk/libsai_impl.a \
  --npu-experiments-path /opt/sdk/experimental \
  --benchmark-install \
  build \
  --allow-system-packages \
  --build-type MinSizeRel \
  --extra-cmake-defines='{"CMAKE_CXX_STANDARD": "20", "RANGE_V3_TESTS": "OFF", "RANGE_V3_PERF": "OFF"}' \
  --scratch-path /var/FBOSS/tmp_bld_dir \
  fboss
