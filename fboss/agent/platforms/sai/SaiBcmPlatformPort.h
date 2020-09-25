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

#include "fboss/agent/platforms/sai/SaiPlatformPort.h"

namespace facebook::fboss {

class SaiBcmPlatformPort : public SaiPlatformPort {
 public:
  explicit SaiBcmPlatformPort(PortID id, SaiPlatform* platform)
      : SaiPlatformPort(id, platform) {}
  virtual uint32_t getPhysicalLaneId(uint32_t chipId, uint32_t logicalLane)
      const override;
  virtual bool supportsTransceiver() const override;
  uint32_t getPhysicalPortId() const;
  void setLEDState(uint32_t led, uint32_t index, uint32_t status);
  uint32_t getLEDState(uint32_t led, uint32_t index);
  uint32_t getCurrentLedState() const override;

 private:
  uint32_t currentLedState_{0};
};

} // namespace facebook::fboss
