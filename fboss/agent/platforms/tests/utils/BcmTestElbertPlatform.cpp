/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/platforms/tests/utils/BcmTestElbertPlatform.h"

#include "fboss/agent/platforms/common/PlatformProductInfo.h"
#include "fboss/agent/platforms/tests/utils/BcmTestElbertPort.h"
#include "fboss/agent/platforms/wedge/elbert/ElbertPlatformMapping.h"

namespace facebook::fboss {

BcmTestElbertPlatform::BcmTestElbertPlatform(
    std::unique_ptr<PlatformProductInfo> productInfo)
    : BcmTestTomahawk4Platform(
          std::move(productInfo),
          std::make_unique<ElbertPlatformMapping>()) {}

std::unique_ptr<BcmTestPort> BcmTestElbertPlatform::createTestPort(PortID id) {
  return std::make_unique<BcmTestElbertPort>(id, this);
}

} // namespace facebook::fboss
