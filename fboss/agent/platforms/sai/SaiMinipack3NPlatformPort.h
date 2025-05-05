/*
 *  Copyright (c) 2023-present, Facebook, Inc.
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

class SaiMinipack3NPlatformPort : public SaiPlatformPort {
 public:
  SaiMinipack3NPlatformPort(const PortID& id, SaiPlatform* platform)
      : SaiPlatformPort(id, platform) {}
  void linkStatusChanged(bool up, bool adminUp) override;
  uint32_t getPhysicalLaneId(uint32_t chipId, uint32_t logicalLane)
      const override;
  //  void externalState(PortLedExternalState lfs) override;
  uint32_t getCurrentLedState() const override;
  void portChanged(
      std::shared_ptr<Port> /*newPort*/,
      std::shared_ptr<Port> /*oldPort*/) override;

  bool supportsTransceiver() const override;

 private:
  uint32_t currentLedState_{0};
};

} // namespace facebook::fboss
