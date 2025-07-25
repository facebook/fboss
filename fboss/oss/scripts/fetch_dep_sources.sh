#!/usr/bin/env bash

# This script needs to be run from the root of the FBOSS OSS github repo.
# It will leverage the getdeps.py script in order to list dependencies and
# fetch the sources to the scratch path.
GETDEPS=./build/fbcode_builder/getdeps.py
$GETDEPS list-deps fboss | while IFS= read -r line; do
  $GETDEPS fetch "$line"
done
