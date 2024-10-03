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

namespace facebook {
namespace fboss {

class WedgeManager;
class PlatformProductInfo;
class FbossMacsecHandler;

std::unique_ptr<WedgeManager> createWedgeManager();

std::shared_ptr<FbossMacsecHandler> createFbossMacsecHandler(
    WedgeManager* wedgeMgr);

/**
 * This function should return derived WedgeManager which is still in dev.
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

std::unique_ptr<WedgeManager> createMeru400bfuWedgeManager(
    const std::string& platformMappingStr);

std::unique_ptr<WedgeManager> createMeru400biaWedgeManager(
    const std::string& platformMappingStr);
std::unique_ptr<WedgeManager> createMeru400biuWedgeManager(
    const std::string& platformMappingStr);
std::unique_ptr<WedgeManager> createMeru800biaWedgeManager(
    const std::string& platformMappingStr);
std::unique_ptr<WedgeManager> createMeru800bfaWedgeManager(
    const std::string& platformMappingStr);

std::unique_ptr<WedgeManager> createMontblancWedgeManager(
    const std::string& platformMappingStr);

std::unique_ptr<WedgeManager> createMorgan800ccWedgeManager(
    const std::string& platformMappingStr);

std::unique_ptr<WedgeManager> createJanga800bicWedgeManager(
    const std::string& platformMappingStr);

std::unique_ptr<WedgeManager> createTahan800bcWedgeManager(
    const std::string& platformMappingStr);

} // namespace fboss
} // namespace facebook
