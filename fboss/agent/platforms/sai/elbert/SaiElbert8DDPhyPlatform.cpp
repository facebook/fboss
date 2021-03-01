/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/sai/elbert/SaiElbert8DDPhyPlatform.h"

#include "fboss/agent/hw/switch_asics/Elbert8DDAsic.h"
#include "fboss/agent/platforms/common/elbert/facebook/Elbert8DDPimPlatformMapping.h"

namespace facebook::fboss {
SaiElbert8DDPhyPlatform::SaiElbert8DDPhyPlatform(
    std::unique_ptr<PlatformProductInfo> productInfo,
    uint8_t pimId,
    int phyId)
    : SaiHwPlatform(
          std::move(productInfo),
          std::make_unique<Elbert8DDPimPlatformMapping>()
              ->getPimPlatformMappingUniquePtr(pimId)),
      pimId_(pimId),
      phyId_(phyId) {
  asic_ = std::make_unique<Elbert8DDAsic>();
}

SaiElbert8DDPhyPlatform::~SaiElbert8DDPhyPlatform() {}

std::string SaiElbert8DDPhyPlatform::getHwConfig() {
  throw FbossError("SaiElbert8DDPhyPlatform doesn't support getHwConfig()");
}
HwAsic* SaiElbert8DDPhyPlatform::getAsic() const {
  return asic_.get();
}
uint32_t SaiElbert8DDPhyPlatform::numLanesPerCore() const {
  throw FbossError("SaiElbert8DDPhyPlatform doesn't support numLanesPerCore()");
}
std::vector<PortID> SaiElbert8DDPhyPlatform::getAllPortsInGroup(
    PortID /* portID */) const {
  throw FbossError("SaiElbert8DDPhyPlatform doesn't support FlexPort");
}
std::vector<FlexPortMode> SaiElbert8DDPhyPlatform::getSupportedFlexPortModes()
    const {
  throw FbossError("SaiElbert8DDPhyPlatform doesn't support FlexPort");
}
std::optional<sai_port_interface_type_t>
SaiElbert8DDPhyPlatform::getInterfaceType(
    TransmitterTechnology /* transmitterTech */,
    cfg::PortSpeed /* speed */) const {
  throw FbossError(
      "SaiElbert8DDPhyPlatform doesn't support getInterfaceType()");
}
bool SaiElbert8DDPhyPlatform::isSerdesApiSupported() {
  return true;
}
bool SaiElbert8DDPhyPlatform::supportInterfaceType() const {
  return false;
}
void SaiElbert8DDPhyPlatform::initLEDs() {
  throw FbossError("SaiElbert8DDPhyPlatform doesn't support initLEDs()");
}
} // namespace facebook::fboss
