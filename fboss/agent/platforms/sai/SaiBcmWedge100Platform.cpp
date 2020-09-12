/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/sai/SaiBcmWedge100Platform.h"

#include "fboss/agent/hw/switch_asics/TomahawkAsic.h"
#include "fboss/agent/platforms/common/utils/Wedge100LedUtils.h"
#include "fboss/agent/platforms/common/wedge100/Wedge100PlatformMapping.h"

#include <cstdio>
#include <cstring>
namespace facebook::fboss {

SaiBcmWedge100Platform::SaiBcmWedge100Platform(
    std::unique_ptr<PlatformProductInfo> productInfo)
    : SaiBcmPlatform(
          std::move(productInfo),
          std::make_unique<Wedge100PlatformMapping>()) {
  asic_ = std::make_unique<TomahawkAsic>();
}

std::vector<PortID> SaiBcmWedge100Platform::masterLogicalPortIds() const {
  constexpr std::array<int, 32> kMasterLogicalPortIds = {
      118, 122, 126, 130, 1,  5,  9,  13, 17, 21, 25, 29, 34,  38,  42,  46,
      50,  54,  58,  62,  68, 72, 76, 80, 84, 88, 92, 96, 102, 106, 110, 114};
  std::vector<PortID> portIds;
  portIds.reserve(kMasterLogicalPortIds.size());
  std::for_each(
      kMasterLogicalPortIds.begin(),
      kMasterLogicalPortIds.end(),
      [&portIds](auto portId) { portIds.emplace_back(PortID(portId)); });
  return portIds;
}

HwAsic* SaiBcmWedge100Platform::getAsic() const {
  return asic_.get();
}

SaiBcmWedge100Platform::~SaiBcmWedge100Platform() {}

void SaiBcmWedge100Platform::initLEDs() {
  initWedgeLED(0, Wedge100LedUtils::defaultLedCode());
  initWedgeLED(1, Wedge100LedUtils::defaultLedCode());
}
} // namespace facebook::fboss
