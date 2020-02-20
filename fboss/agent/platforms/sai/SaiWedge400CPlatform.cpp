/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/sai/SaiWedge400CPlatform.h"
#include "fboss/agent/hw/switch_asics/TajoAsic.h"
#include "fboss/agent/platforms/sai/SaiWedge400CPort.h"

#include <algorithm>

namespace facebook::fboss {

SaiWedge400CPlatform::SaiWedge400CPlatform(
    std::unique_ptr<PlatformProductInfo> productInfo)
    : SaiHwPlatform(std::move(productInfo)) {
  asic_ = std::make_unique<TajoAsic>();
}

std::vector<PortID> SaiWedge400CPlatform::masterLogicalPortIds() const {
  /*
   * First 16 are uplink ports and remaining 32 are downlink ports.
   * The port ID here is only a logical representation of the port and not
   * used by SAI. 8 lanes are used by uplink ports and 4 lanes are used
   * by the downlink ports.
   */

  constexpr auto kNumUplinkPorts{16}, kNumPorts{48};
  auto portIds = std::vector<PortID>(kNumPorts, PortID(0));
  auto currentPortId = 1;
  auto increment = 8;
  auto genNext = [&currentPortId, &increment]() {
    currentPortId += increment;
    auto portId = PortID(currentPortId - increment);
    return portId;
  };
  std::generate_n(portIds.begin(), kNumUplinkPorts, genNext);
  increment = 4;
  std::generate_n(
      portIds.begin() + kNumUplinkPorts, kNumPorts - kNumUplinkPorts, genNext);
  return portIds;
}

std::string SaiWedge400CPlatform::getHwConfig() {
  return config()->thrift.platform.get_chip().get_asic().config;
}

HwAsic* SaiWedge400CPlatform::getAsic() const {
  return asic_.get();
}

SaiWedge400CPlatform::~SaiWedge400CPlatform() {}

} // namespace facebook::fboss
