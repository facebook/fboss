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

class Tomahawk3Asic;

class SaiBcmWedge400Platform : public SaiBcmPlatform {
 public:
  SaiBcmWedge400Platform(
      std::unique_ptr<PlatformProductInfo> productInfo,
      PlatformType type,
      folly::MacAddress localMac,
      const std::string& platformMappingStr);
  ~SaiBcmWedge400Platform() override;
  HwAsic* getAsic() const override;
  uint32_t numLanesPerCore() const override {
    return 8;
  }

  uint32_t numCellsAvailable() const override {
    return 130217;
  }

  std::vector<FlexPortMode> getSupportedFlexPortModes() const override {
    return {
        FlexPortMode::FOURX10G,
        FlexPortMode::FOURX25G,
        FlexPortMode::ONEX40G,
        FlexPortMode::TWOX50G,
        FlexPortMode::ONEX100G,
        FlexPortMode::ONEX400G};
  }

  bool isSerdesApiSupported() const override {
    return true;
  }

  void initLEDs() override;

  std::unique_ptr<PlatformMapping> createWedge400PlatformMapping(
      PlatformType type,
      const std::string& platformMappingStr);

 private:
  void setupAsic(
      cfg::SwitchType switchType,
      std::optional<int64_t> switchId,
      int16_t switchIndex,
      std::optional<cfg::Range64> systemPortRange,
      folly::MacAddress& mac,
      std::optional<HwAsic::FabricNodeRole> fabricNodeRole) override;

  std::unique_ptr<Tomahawk3Asic> asic_;
};

} // namespace facebook::fboss
