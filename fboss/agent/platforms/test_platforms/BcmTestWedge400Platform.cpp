/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/platforms/test_platforms/BcmTestWedge400Platform.h"
#include "fboss/agent/platforms/test_platforms/BcmTestWedge400Port.h"

namespace {
static const std::array<int, 48> kMasterLogicalPortIds = {
    36, 37, 56, 57, 17,  18,  76,  77,  96,  97,  156, 157, 116, 117, 136, 137,
    20, 24, 28, 32, 40,  44,  48,  52,  1,   5,   9,   13,  60,  64,  68,  72,
    80, 84, 88, 92, 140, 144, 148, 152, 100, 104, 108, 112, 120, 124, 128, 132};

constexpr uint8_t kNumPortsPerTransceiver = 1;
} // unnamed namespace

namespace facebook {
namespace fboss {
BcmTestWedge400Platform::BcmTestWedge400Platform()
    : BcmTestWedgeTomahawk3Platform(
          std::vector<PortID>(
              kMasterLogicalPortIds.begin(),
              kMasterLogicalPortIds.end()),
          kNumPortsPerTransceiver) {}

std::unique_ptr<BcmTestPort> BcmTestWedge400Platform::createTestPort(
    PortID id) {
  return std::make_unique<BcmTestWedge400Port>(id, this);
}
} // namespace fboss
} // namespace facebook
