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

class Ramon3Asic;
class Meru800bfaP1PlatformMapping;

class SaiMeru800bfaPlatform : public SaiBcmPlatform {
 public:
  SaiMeru800bfaPlatform(
      std::unique_ptr<PlatformProductInfo> productInfo,
      folly::MacAddress localMac,
      const std::string& platformMappingStr);
  ~SaiMeru800bfaPlatform() override;
  HwAsic* getAsic() const override;

  bool isSerdesApiSupported() const override {
    return true;
  }

  void initLEDs() override;

  std::vector<FlexPortMode> getSupportedFlexPortModes() const override {
    return {};
  }

  bool supportInterfaceType() const override {
    return true;
  }

 protected:
  SaiMeru800bfaPlatform(
      std::unique_ptr<PlatformProductInfo> productInfo,
      std::unique_ptr<Meru800bfaP1PlatformMapping> mapping,
      folly::MacAddress localMac);

 private:
  void setupAsic(
      std::optional<int64_t> switchId,
      const cfg::SwitchInfo& switchInfo,
      std::optional<HwAsic::FabricNodeRole> fabricNodeRole) override;
  std::unique_ptr<Ramon3Asic> asic_;
};

class SaiMeru800P1bfaPlatform : public SaiMeru800bfaPlatform {
 public:
  SaiMeru800P1bfaPlatform(
      std::unique_ptr<PlatformProductInfo> productInfo,
      folly::MacAddress localMac,
      const std::string& platformMappingStr);
};
} // namespace facebook::fboss
