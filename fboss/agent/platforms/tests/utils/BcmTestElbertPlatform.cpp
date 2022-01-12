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

#include "fboss/agent/platforms/common/elbert/Elbert16QPimPlatformMapping.h"
#include "fboss/agent/platforms/tests/utils/BcmTestElbertPort.h"
#include "fboss/lib/platforms/PlatformProductInfo.h"

namespace facebook::fboss {

// This TestPlatform mainly used for bcm_test so that we don't really care
// about how the linecard types are or even whether there's any linecard in the
// chassis. So it's safe to just use Elbert16QPimPlatformMapping
BcmTestElbertPlatform::BcmTestElbertPlatform(
    std::unique_ptr<PlatformProductInfo> productInfo)
    : BcmTestTomahawk4Platform(
          std::move(productInfo),
          std::make_unique<Elbert16QPimPlatformMapping>()) {}

std::unique_ptr<BcmTestPort> BcmTestElbertPlatform::createTestPort(PortID id) {
  return std::make_unique<BcmTestElbertPort>(id, this);
}

} // namespace facebook::fboss
