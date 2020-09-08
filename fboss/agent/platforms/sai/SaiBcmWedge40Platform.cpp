/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/sai/SaiBcmWedge40Platform.h"

#include "fboss/agent/hw/switch_asics/Trident2Asic.h"
#include "fboss/agent/platforms/common/wedge40/Wedge40PlatformMapping.h"

#include "fboss/agent/platforms/common/utils/Wedge40LedUtils.h"

#include <cstdio>
#include <cstring>
namespace facebook::fboss {

SaiBcmWedge40Platform::SaiBcmWedge40Platform(
    std::unique_ptr<PlatformProductInfo> productInfo)
    : SaiBcmPlatform(
          std::move(productInfo),
          std::make_unique<Wedge40PlatformMapping>()) {
  asic_ = std::make_unique<Trident2Asic>();
}

std::vector<PortID> SaiBcmWedge40Platform::masterLogicalPortIds() const {
  constexpr std::array<int, 16> kMasterLogicalPortIds = {
      1, 5, 9, 13, 17, 21, 25, 29, 33, 37, 41, 45, 49, 53, 57, 61};
  std::vector<PortID> portIds;
  portIds.reserve(kMasterLogicalPortIds.size());
  std::for_each(
      kMasterLogicalPortIds.begin(),
      kMasterLogicalPortIds.end(),
      [&portIds](auto portId) { portIds.emplace_back(PortID(portId)); });
  return portIds;
}

HwAsic* SaiBcmWedge40Platform::getAsic() const {
  return asic_.get();
}

SaiBcmWedge40Platform::~SaiBcmWedge40Platform() {}

void SaiBcmWedge40Platform::initLEDs() {
  initWedgeLED(0, Wedge40LedUtils::defaultLedCode());
  initWedgeLED(1, Wedge40LedUtils::defaultLedCode());
}

} // namespace facebook::fboss
