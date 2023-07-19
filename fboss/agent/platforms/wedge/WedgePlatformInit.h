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

#include <folly/MacAddress.h>
#include <memory>

namespace facebook::fboss {

class AgentConfig;
class Platform;
class WedgePlatform;
class PlatformProductInfo;

std::unique_ptr<WedgePlatform> createWedgePlatform();
std::unique_ptr<Platform> initWedgePlatform(
    std::unique_ptr<AgentConfig> config,
    uint32_t hwFeaturesDesired,
    int16_t switchIndex = 0);

/**
 * This function should return derived WedgePlatform which is still in dev.
 * Otherwise, should return null unique_ptr so it can pick the correct
 * well-tested WedgePlatform in createWedgePlatform().
 */
std::unique_ptr<WedgePlatform> createFBWedgePlatform(
    std::unique_ptr<PlatformProductInfo> productInfo,
    folly::MacAddress localMac);

} // namespace facebook::fboss
