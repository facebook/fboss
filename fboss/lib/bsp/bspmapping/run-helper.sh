#!/bin/bash
python3 fboss/lib/oss/run-helper.py \
  --target fboss-bspmapping-gen \
  --extra-cmake-defines='{"RANGE_V3_TESTS":"OFF"}'
