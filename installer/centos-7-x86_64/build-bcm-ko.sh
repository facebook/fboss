#!/usr/bin/env bash

KERNDIR=/usr/src/kernels/$(uname -r)
export KERNDIR

OPENNSADIR=$(./build/fbcode_builder/getdeps.py show-inst-dir OpenNSA "$@")
SDK=$OPENNSADIR/src/gpl-modules
export SDK

cd "$SDK" || return $?
make -C systems/linux/user/common/ platform=x86-smp_generic_64-2_6 kernel_version=2_6 LINUX_UAPI_SPLIT=1 kernel_modules
