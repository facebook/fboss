/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/platforms/sai/SaiCloudRipperPlatformPort.h"
#include "fboss/agent/platforms/sai/SaiCloudRipperPlatform.h"

namespace facebook::fboss {

uint32_t SaiCloudRipperPlatformPort::getPhysicalLaneId(
    uint32_t chipId,
    uint32_t logicalLaneId) const {
  return (chipId << 8) + logicalLaneId;
}

bool SaiCloudRipperPlatformPort::supportsTransceiver() const {
  return true;
}

uint32_t SaiCloudRipperPlatformPort::getCurrentLedState() const {
  // TODO(vsp): LED logic is different for CR compared to Wedge400C, this needs
  // to be re-visited
  return static_cast<uint32_t>(currentLedState_);
}

} // namespace facebook::fboss
