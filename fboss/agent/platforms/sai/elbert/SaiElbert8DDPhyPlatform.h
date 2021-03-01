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

class Elbert8DDAsic;

class SaiElbert8DDPhyPlatform : public SaiHwPlatform {
 public:
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
  bool isSerdesApiSupported() override;
  bool supportInterfaceType() const override;
  void initLEDs() override;

 private:
  uint8_t pimId_{0};
  int phyId_{0};
  std::unique_ptr<Elbert8DDAsic> asic_;
};
} // namespace facebook::fboss
