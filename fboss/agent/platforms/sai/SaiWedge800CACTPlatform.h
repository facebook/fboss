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

#include "fboss/agent/platforms/sai/SaiTajoPlatform.h"

namespace facebook::fboss {

class YubaAsic;

class SaiWedge800CACTPlatform : public SaiTajoPlatform {
 public:
  SaiWedge800CACTPlatform(
      std::unique_ptr<PlatformProductInfo> productInfo,
      folly::MacAddress localMac,
      const std::string& platformMappingStr);
  ~SaiWedge800CACTPlatform() override;
  std::string getHwConfig() override;
  HwAsic* getAsic() const override;

  bool isSerdesApiSupported() const override {
    return true;
  }
  std::vector<PortID> getAllPortsInGroup(PortID) const override;

 private:
  void setupAsic(
      std::optional<int64_t> switchId,
      const cfg::SwitchInfo& switchInfo,
      std::optional<HwAsic::FabricNodeRole> fabricNodeRole) override;
  std::unique_ptr<YubaAsic> asic_;
};

} // namespace facebook::fboss
