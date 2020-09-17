/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/sai/SaiBcmGalaxyLCPlatform.h"
#include "fboss/agent/hw/switch_asics/TomahawkAsic.h"
#include "fboss/agent/platforms/common/galaxy/GalaxyLCPlatformMapping.h"

namespace facebook::fboss {

SaiBcmGalaxyLCPlatform::SaiBcmGalaxyLCPlatform(
    std::unique_ptr<PlatformProductInfo> productInfo)
    : SaiBcmGalaxyPlatform(
          std::move(productInfo),
          std::make_unique<GalaxyLCPlatformMapping>(
              GalaxyLCPlatformMapping::getLinecardName())) {
  asic_ = std::make_unique<TomahawkAsic>();
}

std::vector<PortID> SaiBcmGalaxyLCPlatform::masterLogicalPortIds() const {
  constexpr std::array<int, 32> kMasterLogicalPortIds = {
      84, 88, 92, 96, 102, 106, 110, 114, 118, 122, 126, 130, 1,  5,  9,  13,
      68, 72, 76, 80, 50,  54,  58,  62,  34,  38,  42,  46,  17, 21, 25, 29};
  std::vector<PortID> portIds;
  portIds.reserve(kMasterLogicalPortIds.size());
  std::for_each(
      kMasterLogicalPortIds.begin(),
      kMasterLogicalPortIds.end(),
      [&portIds](auto portId) { portIds.emplace_back(PortID(portId)); });
  return portIds;
}

HwAsic* SaiBcmGalaxyLCPlatform::getAsic() const {
  return asic_.get();
}

SaiBcmGalaxyLCPlatform::~SaiBcmGalaxyLCPlatform() {}
} // namespace facebook::fboss
