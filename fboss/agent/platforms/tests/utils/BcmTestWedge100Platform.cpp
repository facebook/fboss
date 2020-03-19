/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/tests/utils/BcmTestWedge100Platform.h"

#include "fboss/agent/platforms/common/wedge100/Wedge100PlatformMapping.h"
#include "fboss/agent/platforms/tests/utils/BcmTestWedge100Port.h"

namespace facebook::fboss {

BcmTestWedge100Platform::BcmTestWedge100Platform(
    std::unique_ptr<PlatformProductInfo> productInfo)
    : BcmTestWedgeTomahawkPlatform(
          std::move(productInfo),
          std::make_unique<Wedge100PlatformMapping>()) {}

std::unique_ptr<BcmTestPort> BcmTestWedge100Platform::createTestPort(
    PortID id) {
  return std::make_unique<BcmTestWedge100Port>(id, this);
}

} // namespace facebook::fboss
