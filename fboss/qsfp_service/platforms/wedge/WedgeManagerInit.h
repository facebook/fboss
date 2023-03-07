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
    std::unique_ptr<PlatformProductInfo> productInfo);

std::unique_ptr<WedgeManager> createYampWedgeManager();

std::unique_ptr<WedgeManager> createDarwinWedgeManager();

std::unique_ptr<WedgeManager> createLassenWedgeManager();

std::unique_ptr<WedgeManager> createElbertWedgeManager();

bool isElbert8DD();

std::unique_ptr<WedgeManager> createSandiaWedgeManager();

std::unique_ptr<WedgeManager> createMeru400bfuWedgeManager();

std::unique_ptr<WedgeManager> createMeru400biuWedgeManager();

} // namespace fboss
} // namespace facebook
