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

class TomahawkAsic;

class SaiBcmWedge100Platform : public SaiBcmPlatform {
 public:
  SaiBcmWedge100Platform(
      std::unique_ptr<PlatformProductInfo> productInfo,
      folly::MacAddress localMac,
      const std::string& platformMappingStr);
  ~SaiBcmWedge100Platform() override;
  HwAsic* getAsic() const override;
  uint32_t numLanesPerCore() const override {
    return 4;
  }

  uint32_t numCellsAvailable() const override {
    auto constexpr kPerXpeCellsAvailable = 0x436e;
    auto constexpr kPerXpeCellsAvailableOptimized = 0x454A;
    if (getHwConfigValue("buf.mqueue.guarantee.0") &&
        getHwConfigValue("mmu_config_override")) {
      return kPerXpeCellsAvailableOptimized;
    }
    return kPerXpeCellsAvailable;
  }

  std::vector<FlexPortMode> getSupportedFlexPortModes() const override {
    return {
        FlexPortMode::ONEX100G,
        FlexPortMode::TWOX50G,
        FlexPortMode::ONEX40G,
        FlexPortMode::FOURX25G,
        FlexPortMode::FOURX10G};
  }

  void initLEDs() override;

 private:
  void setupAsic(
      cfg::SwitchType switchType,
      std::optional<int64_t> switchId,
      int16_t switchIndex,
      std::optional<cfg::Range64> systemPortRange,
      folly::MacAddress& mac,
      std::optional<HwAsic::FabricNodeRole> fabricNodeRole) override;
  std::unique_ptr<TomahawkAsic> asic_;
};

} // namespace facebook::fboss
