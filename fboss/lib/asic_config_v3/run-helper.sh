#!/bin/bash
exec python3 fboss/lib/oss/run-helper.py \
  --target fboss-asic-config-v3-gen.GEN_PY_EXE \
  --extra-cmake-defines='{"RANGE_V3_TESTS": "OFF"}' \
  --fboss-root fboss \
  "$@"
