#!/bin/bash

FBOSS_BIN=$(dirname "$0")
export FBOSS_BIN
FBOSS=$(dirname "$FBOSS_BIN")
export FBOSS
export FBOSS_LIB=${FBOSS}/lib
export FBOSS_LIB64=${FBOSS}/lib64
export FBOSS_KMODS=${FBOSS}/lib/modules
export FBOSS_DATA=${FBOSS}/share

export PATH=${FBOSS_BIN}${PATH:+:${PATH}}
export LD_LIBRARY_PATH=${FBOSS_LIB}${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH}}
export LD_LIBRARY_PATH=${FBOSS_LIB64}${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH}}

"$FBOSS_BIN"/bcm_test "$@"
