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

#include "fboss/agent/platforms/common/ExternalPhyPort.h"
#include "fboss/agent/platforms/sai/SaiPlatformPort.h"
#include "fboss/agent/platforms/sai/SaiTajoPlatformPort.h"
#include "fboss/lib/fpga/FbDomFpga.h"
#include "fboss/lib/phy/NullPortStats.h"

namespace facebook::fboss {
class SaiCloudRipperPlatform;
class SaiCloudRipperPlatformPort : public SaiTajoPlatformPort,
                                   public ExternalPhyPort<
                                       SaiCloudRipperPlatform,
                                       NullPortStats,
                                       SaiPlatformPort> {
 public:
  explicit SaiCloudRipperPlatformPort(PortID id, SaiPlatform* platform)
      : SaiTajoPlatformPort(id, platform) {}
  uint32_t getCurrentLedState() const override;
  void portChanged(std::shared_ptr<Port> newPort, std::shared_ptr<Port> oldPort)
      override;

 private:
  FbDomFpga::LedColor getLedState(bool up, bool adminUp) const;
  FbDomFpga::LedColor currentLedState_;
};

} // namespace facebook::fboss
