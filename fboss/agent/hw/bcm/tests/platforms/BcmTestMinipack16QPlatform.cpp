/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/tests/platforms/BcmTestMinipack16QPlatform.h"
#include "fboss/agent/hw/bcm/tests/platforms/BcmTestMinipackPort.h"

namespace {
static const std::array<int, 128> kMasterLogicalPortIds = {
  // pim1
  1, 2, 3, 4, 20, 21, 22, 23, 40, 41, 42, 43, 60, 61, 62, 63,
  // pim2
  5, 6, 7, 8, 24, 25, 26, 27, 44, 45, 46, 47, 64, 65, 66, 67,
  // pim3
  9, 10, 11, 12, 28, 29, 30, 31, 48, 49, 50, 51, 68, 69, 70, 71,
  // pim4
  13, 14, 15, 16, 32, 33, 34, 35, 52, 53, 54, 55, 72, 73, 74, 75,
  // pim5
  80, 81, 82, 83, 100, 101, 102, 103, 120, 121, 122, 123, 140, 141, 142, 143,
  // pim6
  84, 85, 86, 87, 104, 105, 106, 107, 124, 125, 126, 127, 144, 145, 146, 147,
  // pim7
  88, 89, 90, 91, 108, 109, 110, 111, 128, 129, 130, 131, 148, 149, 150, 151,
  // pim8
  92, 93, 94, 95, 112, 113, 114, 115, 132, 133, 134, 135, 152, 153, 154, 155
};

constexpr uint8_t kNumPortsPerTransceiver = 1;
} // unnamed namespace

namespace facebook { namespace fboss {
BcmTestMinipack16QPlatform::BcmTestMinipack16QPlatform()
  : BcmTestWedgeTomahawk3Platform(
      std::vector<int>(
        kMasterLogicalPortIds.begin(),
        kMasterLogicalPortIds.end()),
      kNumPortsPerTransceiver) {}

std::unique_ptr<BcmTestPort> BcmTestMinipack16QPlatform::createTestPort(
    PortID id) const {
  return std::make_unique<BcmTestMinipackPort>(id);
}
}} // facebook::fboss
