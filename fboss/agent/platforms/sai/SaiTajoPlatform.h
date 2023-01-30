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

#include "fboss/agent/platforms/sai/SaiHwPlatform.h"

namespace facebook::fboss {

class SaiTajoPlatform : public SaiHwPlatform {
 public:
  explicit SaiTajoPlatform(
      std::unique_ptr<PlatformProductInfo> productInfo,
      std::unique_ptr<PlatformMapping> platformMapping,
      folly::MacAddress localMac);
  ~SaiTajoPlatform() override;

  std::optional<SaiSwitchTraits::Attributes::AclFieldList> getAclFieldList()
      const override;

  std::vector<PortID> getAllPortsInGroup(PortID /*portID*/) const override {
    return {};
  }
  std::vector<FlexPortMode> getSupportedFlexPortModes() const override {
    return {};
  }
  std::optional<sai_port_interface_type_t> getInterfaceType(
      TransmitterTechnology /*transmitterTech*/,
      cfg::PortSpeed /*speed*/) const override {
    return std::nullopt;
  }
  bool isSerdesApiSupported() const override {
    return true;
  }

  bool supportInterfaceType() const override {
    return false;
  }
  void initLEDs() override {}

  const std::unordered_map<std::string, std::string>
  getSaiProfileVendorExtensionValues() const override;
};

} // namespace facebook::fboss
