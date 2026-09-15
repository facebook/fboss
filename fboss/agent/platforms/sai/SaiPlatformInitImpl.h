/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include "fboss/agent/platforms/sai/SaiPlatform.h"
#include "fboss/agent/platforms/sai/SaiPlatformPort.h"

#include <memory>

namespace facebook::fboss {

std::unique_ptr<SaiPlatform> chooseSaiPlatformImpl(
    std::unique_ptr<PlatformProductInfo>& productInfo,
    folly::MacAddress localMac,
    const std::string& platformMappingStr);

std::unique_ptr<SaiPlatformPort> createSaiPlatformPort(
    const PortID& portId,
    SaiPlatform* platform);

} // namespace facebook::fboss
