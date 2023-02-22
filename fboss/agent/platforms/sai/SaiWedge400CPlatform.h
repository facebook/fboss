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

class Wedge400CEbbLabPlatformMapping;
class Wedge400CVoqPlatformMapping;
class Wedge400CFabricPlatformMapping;
class EbroAsic;

class SaiWedge400CPlatform : public SaiTajoPlatform {
 public:
  SaiWedge400CPlatform(
      std::unique_ptr<PlatformProductInfo> productInfo,
      folly::MacAddress localMac,
      const std::string& platformMappingStr);
  ~SaiWedge400CPlatform() override;
  std::string getHwConfig() override;
  HwAsic* getAsic() const override;

 private:
  std::vector<sai_system_port_config_t> getInternalSystemPortConfig()
      const override;

  void setupAsic(
      cfg::SwitchType switchType,
      std::optional<int64_t> switchId,
      std::optional<cfg::Range64> systemPortRange) override;

  std::unique_ptr<PlatformMapping> createWedge400CPlatformMapping(
      const std::string& platformMappingStr);

 protected:
  SaiWedge400CPlatform(
      std::unique_ptr<PlatformProductInfo> productInfo,
      std::unique_ptr<Wedge400CEbbLabPlatformMapping> mapping,
      folly::MacAddress localMac);

  SaiWedge400CPlatform(
      std::unique_ptr<PlatformProductInfo> productInfo,
      std::unique_ptr<Wedge400CVoqPlatformMapping> mapping,
      folly::MacAddress localMac);

  SaiWedge400CPlatform(
      std::unique_ptr<PlatformProductInfo> productInfo,
      std::unique_ptr<Wedge400CFabricPlatformMapping> mapping,
      folly::MacAddress localMac);

  std::unique_ptr<EbroAsic> asic_;
};

class SaiWedge400CEbbLabPlatform : public SaiWedge400CPlatform {
 public:
  SaiWedge400CEbbLabPlatform(
      std::unique_ptr<PlatformProductInfo> productInfo,
      folly::MacAddress localMac,
      const std::string& platformMappingStr);
};

class SaiWedge400CVoqPlatform : public SaiWedge400CPlatform {
 public:
  SaiWedge400CVoqPlatform(
      std::unique_ptr<PlatformProductInfo> productInfo,
      folly::MacAddress localMac,
      const std::string& platformMappingStr);
};

class SaiWedge400CFabricPlatform : public SaiWedge400CPlatform {
 public:
  SaiWedge400CFabricPlatform(
      std::unique_ptr<PlatformProductInfo> productInfo,
      folly::MacAddress localMac,
      const std::string& platformMappingStr);
};

} // namespace facebook::fboss
