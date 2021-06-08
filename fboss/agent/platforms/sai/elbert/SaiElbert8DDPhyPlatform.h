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

class CredoF104Asic;

class SaiElbert8DDPhyPlatform : public SaiHwPlatform {
 public:
  static const std::string& getFirmwareDirectory();

  SaiElbert8DDPhyPlatform(
      std::unique_ptr<PlatformProductInfo> productInfo,
      uint8_t pimId,
      int phyId);
  ~SaiElbert8DDPhyPlatform() override;

  uint8_t getPimId() const {
    return pimId_;
  }

  std::string getHwConfig() override;
  HwAsic* getAsic() const override;
  uint32_t numLanesPerCore() const override;
  std::vector<PortID> getAllPortsInGroup(PortID portID) const override;
  std::vector<FlexPortMode> getSupportedFlexPortModes() const override;
  std::optional<sai_port_interface_type_t> getInterfaceType(
      TransmitterTechnology transmitterTech,
      cfg::PortSpeed speed) const override;
  bool isSerdesApiSupported() const override;
  bool supportInterfaceType() const override;
  void initLEDs() override;

  sai_service_method_table_t* getServiceMethodTable() const override;

  const std::set<sai_api_t>& getSupportedApiList() const override;

  void preHwInitialized() override;

  SaiSwitchTraits::CreateAttributes getSwitchAttributes(
      bool /* mandatoryOnly */) override {
    CHECK(switchCreateAttrs_);
    return *switchCreateAttrs_;
  }
  void setSwitchAttributes(SaiSwitchTraits::CreateAttributes attrs) {
    switchCreateAttrs_ = attrs;
  }

 private:
  uint8_t pimId_{0};
  int phyId_{0};
  std::unique_ptr<CredoF104Asic> asic_;
  std::optional<SaiSwitchTraits::CreateAttributes> switchCreateAttrs_;

  void initImpl(uint32_t hwFeaturesDesired) override;
};
} // namespace facebook::fboss
