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

class ChenabAsic;

class SaiYangraPlatform : public SaiPlatform {
 public:
  SaiYangraPlatform(
      std::unique_ptr<PlatformProductInfo> productInfo,
      folly::MacAddress localMac,
      const std::string& platformMappingStr);
  SaiYangraPlatform(
      std::unique_ptr<PlatformProductInfo> productInfo,
      folly::MacAddress localMac,
      std::unique_ptr<PlatformMapping> platformMapping);
  ~SaiYangraPlatform() override;

  std::optional<SaiSwitchTraits::Attributes::AclFieldList> getAclFieldList()
      const override;

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

  virtual const std::unordered_map<std::string, std::string>
  getSaiProfileVendorExtensionValues() const override;

  std::string getHwConfig() override;

  SaiSwitchTraits::CreateAttributes getSwitchAttributes(
      bool mandatoryOnly,
      cfg::SwitchType switchType,
      std::optional<int64_t> switchId,
      BootType bootType) override;

  std::shared_ptr<apache::thrift::AsyncProcessorFactory> createHandler()
      override {
    return nullptr;
  }

 private:
  void setupAsic(
      std::optional<int64_t> switchId,
      const cfg::SwitchInfo& switchInfo,
      std::optional<HwAsic::FabricNodeRole> role) override;
  std::unique_ptr<ChenabAsic> asic_;
};

} // namespace facebook::fboss
