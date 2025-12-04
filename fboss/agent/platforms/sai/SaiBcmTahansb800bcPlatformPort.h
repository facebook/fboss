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

class SaiBcmTahansb800bcPlatformPort : public SaiBcmPlatformPort {
 public:
  SaiBcmTahansb800bcPlatformPort(PortID id, SaiPlatform* platform)
      : SaiBcmPlatformPort(id, platform) {}

  uint32_t getPhysicalLaneId(uint32_t chipId, uint32_t logicalLane)
      const override {
    if (getPortType() == cfg::PortType::MANAGEMENT_PORT) {
      // The management port core ID is 64
      // However, the correct lane numbering in TH6 start from 514(514-517)
      return SaiBcmPlatformPort::getPhysicalLaneId(chipId, logicalLane) + 1;
    }
    return SaiBcmPlatformPort::getPhysicalLaneId(chipId, logicalLane);
  }

 private:
  uint32_t currentLedState_{0};
};

} // namespace facebook::fboss
