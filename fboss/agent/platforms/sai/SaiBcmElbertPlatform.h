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

namespace facebook::fboss {

class SaiBcmElbertPlatform : public SaiBcmPlatform {
 public:
  SaiBcmElbertPlatform(
      std::unique_ptr<PlatformProductInfo> productInfo,
      folly::MacAddress localMac,
      const std::string& platformMappingStr);
  ~SaiBcmElbertPlatform() override;
  HwAsic* getAsic() const override;
  uint32_t numLanesPerCore() const override {
    return 8;
  }

  uint32_t numCellsAvailable() const override {
    return 223662;
  }

  void initLEDs() override {}

  bool supportsDynamicBcmConfig() const override {
    return true;
  }

 private:
  void setupAsic(
      cfg::SwitchType switchType,
      std::optional<int64_t> switchId,
      int16_t switchIndex,
      std::optional<cfg::Range64> systemPortRange,
      folly::MacAddress& mac,
      std::optional<HwAsic::FabricNodeRole> fabricNodeRole) override;
  std::unique_ptr<Tomahawk4Asic> asic_;
};

} // namespace facebook::fboss
