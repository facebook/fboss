#!/bin/bash
#
# Copyright (c) 2004-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.
#
set -euo pipefail

if [ -f /tmp/.kernel_deps_installed ]; then
  exit 0
fi

dnf install -y --allowerasing \
  rpm-build \
  rpmdevtools \
  rpmlint \
  bc \
  rsync \
  elfutils-libelf-devel \
  elfutils-devel \
  dwarves \
  perl \
  dracut

# Activate GCC 12 toolset if available
# The fboss_builder image has gcc-toolset-12 installed but it needs to be activated
if [ -f /opt/rh/gcc-toolset-12/enable ]; then
  echo "Activating GCC 12 toolset"
  source /opt/rh/gcc-toolset-12/enable
else
  echo "GCC 12 toolset not found - using default GCC"
fi

touch /tmp/.kernel_deps_installed
