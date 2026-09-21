/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/platforms/tests/utils/BcmTestFujiPlatform.h"

#include "fboss/agent/platforms/common/fuji/FujiPlatformMapping.h"
#include "fboss/agent/platforms/tests/utils/BcmTestFujiPort.h"
#include "fboss/lib/platforms/PlatformProductInfo.h"

namespace facebook::fboss {

BcmTestFujiPlatform::BcmTestFujiPlatform(
    std::unique_ptr<PlatformProductInfo> productInfo)
    : BcmTestTomahawk4Platform(
          std::move(productInfo),
          std::make_unique<FujiPlatformMapping>("")) {}

std::unique_ptr<BcmTestPort> BcmTestFujiPlatform::createTestPort(PortID id) {
  return std::make_unique<BcmTestFujiPort>(id, this);
}

} // namespace facebook::fboss
