/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/platforms/test_platforms/BcmTestMinipackPlatform.h"
#include "fboss/agent/platforms/common/PlatformProductInfo.h"
#include "fboss/agent/platforms/test_platforms/BcmTestMinipackPort.h"
#include "fboss/agent/platforms/wedge/minipack/MinipackPlatformMapping.h"

namespace {

constexpr uint8_t kNumPortsPerTransceiver = 1;
} // unnamed namespace

namespace facebook::fboss {

BcmTestMinipackPlatform::BcmTestMinipackPlatform(
    std::unique_ptr<PlatformProductInfo> productInfo)
    : BcmTestWedgeTomahawk3Platform(
          std::move(productInfo),
          std::make_unique<MinipackPlatformMapping>()) {}

std::unique_ptr<BcmTestPort> BcmTestMinipackPlatform::createTestPort(
    PortID id) {
  return std::make_unique<BcmTestMinipackPort>(id, this);
}

} // namespace facebook::fboss
