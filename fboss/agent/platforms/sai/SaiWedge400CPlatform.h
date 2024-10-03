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

class SaiWedge400CPlatform : public SaiTajoPlatform {
 public:
  SaiWedge400CPlatform(
      std::unique_ptr<PlatformProductInfo> productInfo,
      folly::MacAddress localMac,
      const std::string& platformMappingStr);
  ~SaiWedge400CPlatform() override;
  HwAsic* getAsic() const override;
  std::vector<sai_system_port_config_t> getInternalSystemPortConfig()
      const override;
  const std::set<sai_api_t>& getSupportedApiList() const override;

 private:
  void setupAsic(
      cfg::SwitchType switchType,
      std::optional<int64_t> switchId,
      int16_t switchIndex,
      std::optional<cfg::Range64> systemPortRange,
      folly::MacAddress& mac,
      std::optional<HwAsic::FabricNodeRole> fabricNodeRole) override;

  std::unique_ptr<PlatformMapping> createWedge400CPlatformMapping(
      const std::string& platformMappingStr);

 protected:
  std::unique_ptr<EbroAsic> asic_;
};
} // namespace facebook::fboss
