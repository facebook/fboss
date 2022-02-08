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

 private:
  std::unique_ptr<Tomahawk4Asic> asic_;
};

} // namespace facebook::fboss
