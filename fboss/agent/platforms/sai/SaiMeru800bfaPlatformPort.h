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

class SaiMeru800bfaPlatformPort : public SaiBcmPlatformPort {
 public:
  SaiMeru800bfaPlatformPort(PortID id, SaiPlatform* platform)
      : SaiBcmPlatformPort(id, platform) {}
  void linkStatusChanged(bool up, bool adminUp) override;
  void externalState(PortLedExternalState lfs) override;
  uint32_t getCurrentLedState() const override;

  uint32_t getPhysicalLaneId(uint32_t chipId, uint32_t logicalLane)
      const override {
    // Lanes on Meru800bfa platform are 0 indexed
    return SaiBcmPlatformPort::getPhysicalLaneId(chipId, logicalLane) - 1;
  }
  void portChanged(
      std::shared_ptr<Port> /*newPort*/,
      std::shared_ptr<Port> /*oldPort*/) override {}

  virtual bool supportsTransceiver() const override {
    return true;
  }

 private:
  uint32_t currentLedState_{0};
};

} // namespace facebook::fboss
