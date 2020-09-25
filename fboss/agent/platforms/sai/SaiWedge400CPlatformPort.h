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
#include "fboss/lib/fpga/FbDomFpga.h"

namespace facebook::fboss {

class SaiWedge400CPlatformPort : public SaiPlatformPort {
 public:
  explicit SaiWedge400CPlatformPort(PortID id, SaiPlatform* platform)
      : SaiPlatformPort(id, platform) {}
  virtual uint32_t getPhysicalLaneId(uint32_t chipId, uint32_t logicalLane)
      const override;
  virtual bool supportsTransceiver() const override;
  void linkStatusChanged(bool up, bool adminUp) override;
  void externalState(PortLedExternalState lfs) override;
  uint32_t getCurrentLedState() const override;

 private:
  FbDomFpga::LedColor getLedState(bool up, bool adminUp) const;
  void setLedStatus(FbDomFpga::LedColor state) const;
  FbDomFpga::LedColor currentLedState_;
};

} // namespace facebook::fboss
