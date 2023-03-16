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

#include <memory>

namespace facebook::fboss {

class AgentConfig;
class Platform;
class SaiPlatform;
class PlatformProductInfo;
class SaiWedge400CPlatform;

std::unique_ptr<SaiPlatform> chooseSaiPlatform(
    std::unique_ptr<PlatformProductInfo> productIfo,
    folly::MacAddress localMac,
    const std::string& platformMappingStr);

std::unique_ptr<Platform> initSaiPlatform(
    std::unique_ptr<AgentConfig> config,
    uint32_t hwFeaturesDesired);

std::unique_ptr<SaiPlatform> getLEBPlatform(
    std::unique_ptr<PlatformProductInfo> productInfo,
    folly::MacAddress localMac,
    const std::string& platformMappingStr);

} // namespace facebook::fboss
