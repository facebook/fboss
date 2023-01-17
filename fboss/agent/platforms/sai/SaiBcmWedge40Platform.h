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

#include "fboss/agent/platforms/sai/SaiBcmPlatform.h"
namespace facebook::fboss {

class Trident2Asic;

class SaiBcmWedge40Platform : public SaiBcmPlatform {
 public:
  explicit SaiBcmWedge40Platform(
      std::unique_ptr<PlatformProductInfo> productInfo,
      folly::MacAddress localMac);
  ~SaiBcmWedge40Platform() override;
  HwAsic* getAsic() const override;
  uint32_t numLanesPerCore() const override {
    return 4;
  }

  uint32_t numCellsAvailable() const override {
    return 48399;
  }

  std::vector<FlexPortMode> getSupportedFlexPortModes() const override {
    return {FlexPortMode::ONEX40G, FlexPortMode::FOURX10G};
  }
  void initLEDs() override;

 private:
  void setupAsic(
      cfg::SwitchType switchType,
      std::optional<int64_t> switchId,
      std::optional<cfg::Range64> systemPortRange) override;
  std::unique_ptr<Trident2Asic> asic_;
};

} // namespace facebook::fboss
