/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/platforms/sai/SaiBcmPlatformPort.h"
#include "fboss/agent/platforms/sai/SaiPlatform.h"

namespace facebook::fboss {

uint32_t SaiBcmPlatformPort::getPhysicalLaneId(
    uint32_t chipId,
    uint32_t logicalLane) const {
  auto platform = static_cast<SaiPlatform*>(getPlatform());
  return chipId * platform->numLanesPerCore() + logicalLane + 1;
}

bool SaiBcmPlatformPort::supportsTransceiver() const {
  return true;
}

} // namespace facebook::fboss
