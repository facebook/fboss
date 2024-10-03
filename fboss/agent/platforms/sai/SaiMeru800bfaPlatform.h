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

  uint32_t numLanesPerCore() const override {
    return 8;
  }

  uint32_t numCellsAvailable() const override {
    return 130665;
  }

  bool isSerdesApiSupported() const override {
    return true;
  }

  void initLEDs() override;

  std::vector<PortID> getAllPortsInGroup(PortID /*portID*/) const override {
    return {};
  }

  std::vector<FlexPortMode> getSupportedFlexPortModes() const override {
    return {};
  }

  bool supportInterfaceType() const override {
    return true;
  }

  std::optional<sai_port_interface_type_t> getInterfaceType(
      TransmitterTechnology /*transmitterTech*/,
      cfg::PortSpeed /*speed*/) const override {
    return std::nullopt;
  }

 protected:
  SaiMeru800bfaPlatform(
      std::unique_ptr<PlatformProductInfo> productInfo,
      std::unique_ptr<Meru800bfaP1PlatformMapping> mapping,
      folly::MacAddress localMac);

 private:
  void setupAsic(
      cfg::SwitchType switchType,
      std::optional<int64_t> switchId,
      int16_t switchIndex,
      std::optional<cfg::Range64> systemPortRange,
      folly::MacAddress& mac,
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
