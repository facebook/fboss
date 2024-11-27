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

class EbroAsic;

class SaiCloudRipperPlatform : public SaiTajoPlatform {
 public:
  SaiCloudRipperPlatform(
      std::unique_ptr<PlatformProductInfo> productInfo,
      folly::MacAddress localMac,
      const std::string& platformMappingStr);
  ~SaiCloudRipperPlatform() override;
  HwAsic* getAsic() const override;

  std::vector<sai_system_port_config_t> getInternalSystemPortConfig()
      const override;
  const std::set<sai_api_t>& getSupportedApiList() const override;

 private:
  void setupAsic(
      std::optional<int64_t> switchId,
      const cfg::SwitchInfo& switchInfo,
      std::optional<HwAsic::FabricNodeRole> fabricNodeRole) override;
  std::unique_ptr<EbroAsic> asic_;
};
} // namespace facebook::fboss
