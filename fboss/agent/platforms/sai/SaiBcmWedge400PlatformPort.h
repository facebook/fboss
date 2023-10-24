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
#include "fboss/lib/fpga/FbDomFpga.h"

namespace facebook::fboss {

class SaiBcmWedge400PlatformPort : public SaiBcmPlatformPort {
 public:
  SaiBcmWedge400PlatformPort(PortID id, SaiPlatform* platform)
      : SaiBcmPlatformPort(id, platform) {}
  void linkStatusChanged(bool up, bool adminUp) override;
  void externalState(PortLedExternalState lfs) override;
  uint32_t getCurrentLedState() const override;

 private:
  void setLedStatus(FbDomFpga::LedColor state) const;
  FbDomFpga::LedColor currentLedState_{FbDomFpga::LedColor::OFF};
};

} // namespace facebook::fboss
