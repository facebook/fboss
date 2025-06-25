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

#include <memory>
#include "fboss/agent/platforms/common/PlatformMapping.h"
#include "fboss/lib/if/gen-cpp2/fboss_common_types.h"

namespace facebook {
namespace fboss {

class WedgeManager;
class PlatformProductInfo;
class FbossMacsecHandler;

std::unique_ptr<WedgeManager> createWedgeManager();

std::shared_ptr<FbossMacsecHandler> createFbossMacsecHandler(
    WedgeManager* wedgeMgr);

/**
 * These functions enable us to create different WedgeManagers for open-sourced
 * and closed-souce platforms.
 */
std::unique_ptr<WedgeManager> createFBWedgeManager(
    std::unique_ptr<PlatformProductInfo> productInfo,
    const std::string& platformMappingStr);

std::unique_ptr<WedgeManager> createYampWedgeManager(
    const std::string& platformMappingStr);

std::unique_ptr<WedgeManager> createDarwinWedgeManager(
    const std::string& platformMappingStr);

std::unique_ptr<WedgeManager> createElbertWedgeManager(
    const std::string& platformMappingStr);

bool isElbert8DD();

std::string getDeviceDatacenter();
std::string getDeviceHostnameScheme();

template <
    typename BspPlatformMapping,
    typename PlatformMapping,
    PlatformType platformType>
std::unique_ptr<WedgeManager> createBspWedgeManager(
    const std::string& platformMappingStr);

} // namespace fboss
} // namespace facebook
