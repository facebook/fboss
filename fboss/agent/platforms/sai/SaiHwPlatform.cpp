/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/sai/SaiHwPlatform.h"

#include <cstdio>
#include <cstring>
namespace facebook::fboss {

DEFINE_string(
    volatile_state_dir,
    "/dev/shm/fboss",
    "Directory for storing volatile state");
DEFINE_string(
    persistent_state_dir,
    "/var/facebook/fboss",
    "Directory for storing persistent state");

std::string SaiHwPlatform::getVolatileStateDir() const {
  return FLAGS_volatile_state_dir;
}

std::string SaiHwPlatform::getPersistentStateDir() const {
  return FLAGS_persistent_state_dir;
}

} // namespace facebook::fboss
