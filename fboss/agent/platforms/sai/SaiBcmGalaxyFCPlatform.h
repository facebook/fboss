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

#include "fboss/agent/platforms/sai/SaiBcmGalaxyPlatform.h"
namespace facebook::fboss {

class TomahawkAsic;

class SaiBcmGalaxyFCPlatform : public SaiBcmGalaxyPlatform {
 public:
  explicit SaiBcmGalaxyFCPlatform(
      std::unique_ptr<PlatformProductInfo> productInfo,
      folly::MacAddress localMac);
  ~SaiBcmGalaxyFCPlatform() override;
  HwAsic* getAsic() const override;
  std::vector<FlexPortMode> getSupportedFlexPortModes() const override {
    return {FlexPortMode::ONEX100G};
  }

 private:
  void setupAsic(
      cfg::SwitchType switchType,
      std::optional<int64_t> switchId,
      std::optional<cfg::Range64> systemPortRange) override;
  std::unique_ptr<TomahawkAsic> asic_;
};

} // namespace facebook::fboss
