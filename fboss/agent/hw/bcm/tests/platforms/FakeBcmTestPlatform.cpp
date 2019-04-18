/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/tests/platforms/FakeBcmTestPlatform.h"
#include "fboss/agent/hw/bcm/tests/platforms/FakeBcmTestPort.h"

using std::make_unique;

namespace {
static const std::array<int, 8> kMasterLogicalPortIds =
    {1, 5, 9, 13, 17, 21, 25, 29};
constexpr uint8_t kNumPortsPerTransceiver = 4;
} // namespace

namespace facebook {
namespace fboss {
FakeBcmTestPlatform::FakeBcmTestPlatform()
    : BcmTestPlatform(
          std::vector<PortID>(
              kMasterLogicalPortIds.begin(),
              kMasterLogicalPortIds.end()),
          kNumPortsPerTransceiver) {}

std::unique_ptr<BcmTestPort> FakeBcmTestPlatform::createTestPort(
    PortID id) const {
  return std::make_unique<FakeBcmTestPort>(id);
}

std::string FakeBcmTestPlatform::getVolatileStateDir() const {
  return tmpDir_.path().string() + "/volatile";
}

std::string FakeBcmTestPlatform::getPersistentStateDir() const {
  return tmpDir_.path().string() + "/persist";
}

} // namespace fboss
} // namespace facebook
