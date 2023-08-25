#!/bin/bash

if [[ "$BUCK_DEFAULT_RUNTIME_RESOURCES" == "" ]]; then
    echo "Error: must be run via buck target //fboss/fsdb/if/oss:sync_model_thriftpath"
    exit 1
fi

TARGET_PATH_REL="fboss/fsdb/if/oss/fsdb_model_thriftpath.h"
# shellcheck disable=SC2001
FBSOURCE_ROOT=$(echo "$BUCK_DEFAULT_RUNTIME_RESOURCES" | sed 's:\(.*fbsource\)/.*:\1:')
FBCODE_ROOT="$FBSOURCE_ROOT/fbcode"
SRC="$BUCK_DEFAULT_RUNTIME_RESOURCES/$TARGET_PATH_REL"
DST="$FBCODE_ROOT/$TARGET_PATH_REL"

echo "Copying $SRC to $DST"
cp "$SRC" "$DST"
