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

#include "fboss/agent/platforms/sai/SaiPlatform.h"

namespace facebook::fboss {

class GenericSaiYangraPlatform : public SaiPlatform {
 public:
  GenericSaiYangraPlatform(
      std::unique_ptr<PlatformProductInfo> productInfo,
      std::unique_ptr<PlatformMapping> platformMapping,
      folly::MacAddress localMac);
  ~GenericSaiYangraPlatform() override;

  HwAsic* getAsic() const override;
  bool isSerdesApiSupported() const override;
  std::vector<PortID> getAllPortsInGroup(PortID /*portID*/) const override;
  std::vector<FlexPortMode> getSupportedFlexPortModes() const override;
  std::optional<sai_port_interface_type_t> getInterfaceType(
      TransmitterTechnology /*transmitterTech*/,
      cfg::PortSpeed /*speed*/) const override;
  bool supportInterfaceType() const override;
  void initLEDs() override;

  const std::set<sai_api_t>& getSupportedApiList() const override;

  const std::unordered_map<std::string, std::string>
  getSaiProfileVendorExtensionValues() const override;

  std::string getHwConfig() override;

  SaiSwitchTraits::CreateAttributes getSwitchAttributes(
      bool mandatoryOnly,
      cfg::SwitchType switchType,
      std::optional<int64_t> switchId,
      BootType bootType) override;
  HwSwitchWarmBootHelper* getWarmBootHelper() override;

 private:
  void setupAsic(
      std::optional<int64_t> switchId,
      const cfg::SwitchInfo& switchInfo,
      std::optional<HwAsic::FabricNodeRole> fabricNodeRole) override;

  std::unique_ptr<HwAsic> asic_;
};

} // namespace facebook::fboss
