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

#include "fboss/agent/hw/switch_asics/Tomahawk4Asic.h"
#include "fboss/agent/platforms/sai/SaiBcmPlatform.h"
#include "fboss/lib/phy/PhyInterfaceHandler.h"

namespace facebook::fboss {

class SaiBcmFujiPlatform : public SaiBcmPlatform {
 public:
  explicit SaiBcmFujiPlatform(
      std::unique_ptr<PlatformProductInfo> productInfo,
      folly::MacAddress localMac);
  ~SaiBcmFujiPlatform() override;
  HwAsic* getAsic() const override;
  uint32_t numLanesPerCore() const override {
    return 8;
  }

  uint32_t numCellsAvailable() const override {
    // This depends on bcm config, see D33198369
    // Currently return a random value, will update it later
    return 127977;
  }

  void initLEDs() override {}

  PortID findPortID(
      cfg::PortSpeed speed,
      std::vector<uint32_t> lanes,
      PortSaiId portSaiId) const override {
    for (const auto& portMapping : portMapping_) {
      const auto& platformPort = portMapping.second;
      // TODO(daiweix): remove this method and use lanes info to find port ID
      // after lane attribute get issue in CS00012236630 is resolved.
      // For now, dervide port id from the lower 32 bits of portSaiId, which
      // is the broadcom logical port id
      //  +--------------|---------|---------|-----------------------+
      //  |63..........48|47.....40|39.....32|31....................0|
      //  +--------------|---------|---------|-----------------------+
      //  |     map1     | subtype |   type  |           id          |
      //  |              | or map2 |         |                       |
      //  +--------------|---------|---------|-----------------------+
      if (platformPort->getPortID() == (PortID)portSaiId) {
        return platformPort->getPortID();
      }
    }
    throw FbossError("platform port not found ", (PortID)portSaiId);
  }

 private:
  std::unique_ptr<Tomahawk4Asic> asic_;
};

} // namespace facebook::fboss
