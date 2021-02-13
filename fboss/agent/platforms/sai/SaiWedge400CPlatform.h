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

class TajoAsic;
class Wedge400CEbbLabPlatformMapping;

class SaiWedge400CPlatform : public SaiHwPlatform {
 public:
  explicit SaiWedge400CPlatform(
      std::unique_ptr<PlatformProductInfo> productInfo);
  ~SaiWedge400CPlatform() override;
  std::string getHwConfig() override;
  HwAsic* getAsic() const override;
  uint32_t numLanesPerCore() const override {
    return 4;
  }
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
  bool isSerdesApiSupported() override {
    return true;
  }

  bool supportInterfaceType() const override {
    return false;
  }
  void initLEDs() override {}

  std::optional<SaiSwitchTraits::Attributes::AclFieldList> getAclFieldList()
      const override;

 protected:
  SaiWedge400CPlatform(
      std::unique_ptr<PlatformProductInfo> productInfo,
      std::unique_ptr<Wedge400CEbbLabPlatformMapping> mapping);

 private:
  std::unique_ptr<TajoAsic> asic_;
};

class SaiWedge400CEbbLabPlatform : public SaiWedge400CPlatform {
 public:
  explicit SaiWedge400CEbbLabPlatform(
      std::unique_ptr<PlatformProductInfo> productInfo);
};

} // namespace facebook::fboss
