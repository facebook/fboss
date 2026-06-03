#!/bin/bash

set -e

scratch_path="/var/FBOSS/scratch"
cd /var/FBOSS/fboss || exit

job_count="17"
build_type="MinSizeRel"
cmake_targets=("sai_test-sai_impl" "sai_agent_hw_test-sai_impl" "sai_agent_scale_test-sai_impl" "sai_replayer-sai_impl" "fboss2")

if [ -f ./fboss_env.sh ]; then
    source ./fboss_env.sh
else
    echo "File ./fboss_env.sh not found !!!!"
    exit -1
fi

ulimit -v unlimited

./fboss/oss/scripts/run-getdeps.py install-system-deps --recursive fboss

if [ "${BUILD_LINK_BINARIES:-0}" = "1" ]; then
    cmake_targets+=("qsfp_service" "wedge_qsfp_util" "fsdb" "sai_mono_link_test-sai_impl")
fi

if [ -n "$NUM_JOBS" ] && [ "$NUM_JOBS" -ne 0 ]; then
    job_count="$NUM_JOBS"
fi

for cmake_target in "${cmake_targets[@]}"; do
    ./fboss/oss/scripts/run-getdeps.py build fboss --allow-system-packages --num-jobs $job_count --build-type $build_type --extra-cmake-defines='{"CMAKE_CXX_STANDARD": "20", "RANGE_V3_TESTS": "OFF", "RANGE_V3_PERF": "OFF"}' --scratch-path $scratch_path --cmake-target $cmake_target
done

rm -rf /var/FBOSS/scratch/fboss_bins*

./fboss/oss/scripts/package-fboss.py --copy-root-libs --scratch-path=/var/FBOSS/scratch

cp -Lv /usr/lib64/libpython3.12.so.1.0 $(find /var/ -name fboss_bins*)/lib/
cp -Lv /usr/lib64/libaio.so.1 $(find /var/ -name fboss_bins*)/lib/
\cp -Lv $(find /var/FBOSS/scratch/installed -name libsai*.so) $(find /var/ -name fboss_bins*)/lib/
\cp -Lv -R /opt/grpc/lib/* $(find /var/ -name fboss_bins*)/lib/
\cp -Lv -R /opt/sdk_build/boost/lib/* $(find /var/ -name fboss_bins*)/lib/
\cp -Lv -R /opt/sdk_build/klish/.libs/* $(find /var/ -name fboss_bins*)/lib/
