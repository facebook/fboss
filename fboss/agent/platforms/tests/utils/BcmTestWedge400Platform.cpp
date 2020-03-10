/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/platforms/tests/utils/BcmTestWedge400Platform.h"
#include "fboss/agent/platforms/common/PlatformProductInfo.h"
#include "fboss/agent/platforms/tests/utils/BcmTestWedge400Port.h"
#include "fboss/agent/platforms/wedge/wedge400/Wedge400PlatformMapping.h"

namespace facebook::fboss {

BcmTestWedge400Platform::BcmTestWedge400Platform(
    std::unique_ptr<PlatformProductInfo> productInfo)
    : BcmTestWedgeTomahawk3Platform(
          std::move(productInfo),
          std::make_unique<Wedge400PlatformMapping>()) {}

std::unique_ptr<BcmTestPort> BcmTestWedge400Platform::createTestPort(
    PortID id) {
  return std::make_unique<BcmTestWedge400Port>(id, this);
}

} // namespace facebook::fboss
