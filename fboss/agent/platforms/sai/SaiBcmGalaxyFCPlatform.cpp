/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/sai/SaiBcmGalaxyFCPlatform.h"
#include "fboss/agent/hw/switch_asics/TomahawkAsic.h"
#include "fboss/agent/platforms/common/galaxy/GalaxyFCPlatformMapping.h"

namespace facebook::fboss {

SaiBcmGalaxyFCPlatform::SaiBcmGalaxyFCPlatform(
    std::unique_ptr<PlatformProductInfo> productInfo)
    : SaiBcmGalaxyPlatform(
          std::move(productInfo),
          std::make_unique<GalaxyFCPlatformMapping>(
              GalaxyFCPlatformMapping::getFabriccardName())) {
  asic_ = std::make_unique<TomahawkAsic>();
}

std::vector<PortID> SaiBcmGalaxyFCPlatform::masterLogicalPortIds() const {
  constexpr std::array<int, 32> kMasterLogicalPortIds = {
      72, 76, 68, 80, 62,  58,  54,  50,  110, 106, 102, 114, 96, 92, 84, 88,
      5,  9,  1,  13, 130, 118, 122, 126, 42,  38,  29,  46,  34, 21, 25, 17};
  std::vector<PortID> portIds;
  portIds.reserve(kMasterLogicalPortIds.size());
  std::for_each(
      kMasterLogicalPortIds.begin(),
      kMasterLogicalPortIds.end(),
      [&portIds](auto portId) { portIds.emplace_back(PortID(portId)); });
  return portIds;
}

HwAsic* SaiBcmGalaxyFCPlatform::getAsic() const {
  return asic_.get();
}

SaiBcmGalaxyFCPlatform::~SaiBcmGalaxyFCPlatform() {}
} // namespace facebook::fboss
