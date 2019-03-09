/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/bcm/tests/platforms/BcmTestWedgePlatform.h"

DEFINE_string(volatile_state_dir, "/dev/shm/fboss/bcm_test",
              "Directory for storing volatile state");
DEFINE_string(persistent_state_dir, "/var/facebook/fboss/bcm_test",
              "Directory for storing persistent state");

namespace facebook {
namespace fboss {

BcmTestWedgePlatform::BcmTestWedgePlatform(
    std::vector<int> masterLogicalPortIds,
    int numPortsPerTranceiver)
    : BcmTestPlatform(masterLogicalPortIds, numPortsPerTranceiver) {}

std::string BcmTestWedgePlatform::getVolatileStateDir() const {
  return FLAGS_volatile_state_dir;
}

std::string BcmTestWedgePlatform::getPersistentStateDir() const {
  return FLAGS_persistent_state_dir;
}

} // namespace fboss
} // namespace facebook
