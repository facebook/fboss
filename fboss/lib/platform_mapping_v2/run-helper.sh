#!/bin/bash
python3 fboss/lib/oss/run-helper.py \
  --target fboss-platform-mapping-gen.GEN_PY_EXE \
  --extra-cmake-defines='{"RANGE_V3_TESTS":"OFF"}' \
  "$@"
