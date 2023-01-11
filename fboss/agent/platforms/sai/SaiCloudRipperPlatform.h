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
class CloudRipperVoqPlatformMapping;
class CloudRipperFabricPlatformMapping;

class SaiCloudRipperPlatform : public SaiTajoPlatform {
 public:
  explicit SaiCloudRipperPlatform(
      std::unique_ptr<PlatformProductInfo> productInfo,
      folly::MacAddress localMac);
  SaiCloudRipperPlatform(
      std::unique_ptr<PlatformProductInfo> productInfo,
      std::unique_ptr<CloudRipperVoqPlatformMapping> mapping,
      folly::MacAddress localMac);

  SaiCloudRipperPlatform(
      std::unique_ptr<PlatformProductInfo> productInfo,
      std::unique_ptr<CloudRipperFabricPlatformMapping> mapping,
      folly::MacAddress localMac);
  ~SaiCloudRipperPlatform() override;
  HwAsic* getAsic() const override;

  std::string getHwConfig() override;

 private:
  void setupAsic(cfg::SwitchType switchType, std::optional<int64_t> switchId)
      override;
  std::vector<sai_system_port_config_t> getInternalSystemPortConfig()
      const override;
  std::unique_ptr<EbroAsic> asic_;
};

class SaiCloudRipperVoqPlatform : public SaiCloudRipperPlatform {
 public:
  SaiCloudRipperVoqPlatform(
      std::unique_ptr<PlatformProductInfo> productInfo,
      folly::MacAddress localMac);
};

class SaiCloudRipperFabricPlatform : public SaiCloudRipperPlatform {
 public:
  SaiCloudRipperFabricPlatform(
      std::unique_ptr<PlatformProductInfo> productInfo,
      folly::MacAddress localMac);
};

} // namespace facebook::fboss
