/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/bcm/tests/platforms/BcmTestWedge100Platform.h"

#include "fboss/agent/hw/bcm/tests/platforms/BcmTestWedge100Port.h"

using std::make_unique;

namespace {
static const std::array<int, 32> kMasterLogicalPortIds = {
  118, 122, 126, 130, 1, 5, 9, 13, 17, 21, 25, 29, 34, 38, 42, 46, 50, 54, 58,
  62, 68, 72, 76, 80, 84, 88, 92, 96, 102, 106, 110, 114
};

constexpr uint8_t kNumPortsPerTransceiver = 4;
}

namespace facebook {
namespace fboss {
BcmTestWedge100Platform::BcmTestWedge100Platform()
    : BcmTestWedgeTomahawkPlatform(
          std::vector<int>(
              kMasterLogicalPortIds.begin(),
              kMasterLogicalPortIds.end()),
          kNumPortsPerTransceiver) {}

std::unique_ptr<BcmTestPort> BcmTestWedge100Platform::getPlatformPort(
    PortID id) {
  return make_unique<BcmTestWedge100Port>(id);
}

} // namespace fboss
} // namespace facebook
