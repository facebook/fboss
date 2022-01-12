/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/platforms/tests/utils/BcmTestMinipackPlatform.h"

#include "fboss/agent/platforms/common/minipack/Minipack16QPimPlatformMapping.h"
#include "fboss/agent/platforms/tests/utils/BcmTestMinipackPort.h"
#include "fboss/lib/platforms/PlatformProductInfo.h"

namespace facebook::fboss {

// This TestPlatform mainly used for bcm_test so that we don't really care
// about how the linecard types are or even whether there's any linecard in the
// chassis. So it's safe to just use Minipack16QPimPlatformMapping.
BcmTestMinipackPlatform::BcmTestMinipackPlatform(
    std::unique_ptr<PlatformProductInfo> productInfo)
    : BcmTestWedgeTomahawk3Platform(
          std::move(productInfo),
          std::make_unique<Minipack16QPimPlatformMapping>(
              ExternalPhyVersion::MILN5_2)) {}

std::unique_ptr<BcmTestPort> BcmTestMinipackPlatform::createTestPort(
    PortID id) {
  return std::make_unique<BcmTestMinipackPort>(id, this);
}

} // namespace facebook::fboss
