/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include "fboss/agent/platforms/sai/SaiBcmPlatformPort.h"

namespace facebook::fboss {

class SaiBcmJ4SimPlatformPort : public SaiBcmPlatformPort {
 public:
  SaiBcmJ4SimPlatformPort(const PortID& id, SaiPlatform* platform)
      : SaiBcmPlatformPort(id, platform) {}
  void linkStatusChanged(bool up, bool adminUp) override;
  void externalState(PortLedExternalState lfs) override;

  uint32_t getPhysicalLaneId(uint32_t chipId, uint32_t logicalLane)
      const override {
    if (getPortType() != cfg::PortType::RECYCLE_PORT &&
        getPortType() != cfg::PortType::EVENTOR_PORT) {
      // Lanes on J4 platform are 0 indexed
      return SaiBcmPlatformPort::getPhysicalLaneId(chipId, logicalLane) - 1;
    }
    // Recycle port lanes are 1 indexed
    return chipId;
  }
};

} // namespace facebook::fboss
