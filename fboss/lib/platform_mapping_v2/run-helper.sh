#!/bin/bash
exec python3 fboss/lib/oss/run-helper.py \
  --target fboss-platform-mapping-gen \
  --fboss-root fboss \
  "$@"
