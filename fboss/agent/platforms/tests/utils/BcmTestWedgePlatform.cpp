/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/tests/utils/BcmTestWedgePlatform.h"

#include "fboss/lib/platforms/PlatformProductInfo.h"

namespace facebook::fboss {

BcmTestWedgePlatform::BcmTestWedgePlatform(
    std::unique_ptr<PlatformProductInfo> productInfo,
    std::unique_ptr<PlatformMapping> platformMapping)
    : BcmTestPlatform(std::move(productInfo), std::move(platformMapping)) {}

} // namespace facebook::fboss
