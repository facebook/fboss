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

class CredoPhyAsic;

class SaiCloudRipperPhyPlatform : public SaiHwPlatform {
 public:
  static const std::string& getFirmwareDirectory();

  SaiCloudRipperPhyPlatform(
      std::unique_ptr<PlatformProductInfo> productInfo,
      folly::MacAddress localMac,
      int phyId);
  ~SaiCloudRipperPhyPlatform() override;

  std::string getHwConfig() override;
  HwAsic* getAsic() const override;
  std::vector<PortID> getAllPortsInGroup(PortID portID) const override;
  std::vector<FlexPortMode> getSupportedFlexPortModes() const override;
  std::optional<sai_port_interface_type_t> getInterfaceType(
      TransmitterTechnology transmitterTech,
      cfg::PortSpeed speed) const override;
  bool isSerdesApiSupported() const override;
  bool supportInterfaceType() const override;
  void initLEDs() override;
  std::string getPersistentStateDir() const override {
    return FLAGS_persistent_state_dir_phy + "/" +
        folly::to<std::string>(phyId_);
  }
  std::string getVolatileStateDir() const override {
    return FLAGS_volatile_state_dir_phy + "/" + folly::to<std::string>(phyId_);
  }

  sai_service_method_table_t* getServiceMethodTable() const override;

  const std::set<sai_api_t>& getSupportedApiList() const override;

  void preHwInitialized() override;

  SaiSwitchTraits::CreateAttributes getSwitchAttributes(
      bool /*mandatoryOnly*/,
      cfg::SwitchType switchType,
      std::optional<int64_t> switchId) override {
    CHECK(switchCreateAttrs_);
    return *switchCreateAttrs_;
  }
  void setSwitchAttributes(SaiSwitchTraits::CreateAttributes attrs) {
    switchCreateAttrs_ = attrs;
  }

 private:
  void setupAsic(
      cfg::SwitchType switchType,
      std::optional<int64_t> switchId,
      std::optional<cfg::Range64> systemPortRange,
      folly::MacAddress& mac) override;
  int phyId_{0};
  std::unique_ptr<CredoPhyAsic> asic_;
  std::optional<SaiSwitchTraits::CreateAttributes> switchCreateAttrs_;

  void initImpl(uint32_t hwFeaturesDesired) override;
};
} // namespace facebook::fboss
