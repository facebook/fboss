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

class SaiBcmPlatform : public SaiHwPlatform {
 public:
  using SaiHwPlatform::SaiHwPlatform;
  std::string getHwConfig() override;
  bool getObjectKeysSupported() const override {
    return true;
  }
  std::vector<PortID> getAllPortsInGroup(PortID portID) const override;

  std::vector<FlexPortMode> getSupportedFlexPortModes() const override {
    // TODO: implement this
    return {};
  }
  std::optional<sai_port_interface_type_t> getInterfaceType(
      TransmitterTechnology transmitterTech,
      cfg::PortSpeed speed) const override;
  bool isSerdesApiSupported() override {
    return false;
  }
  bool supportInterfaceType() const override {
    return true;
  }
};

} // namespace facebook::fboss
