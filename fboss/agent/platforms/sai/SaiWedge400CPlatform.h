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

class SaiWedge400CPlatform : public SaiHwPlatform {
 public:
  explicit SaiWedge400CPlatform(
      std::unique_ptr<PlatformProductInfo> productInfo);
  ~SaiWedge400CPlatform() override;
  std::vector<PortID> masterLogicalPortIds() const override;
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

 private:
  std::unique_ptr<TajoAsic> asic_;
};

} // namespace facebook::fboss
