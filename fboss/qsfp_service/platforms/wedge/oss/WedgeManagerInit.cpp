/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/qsfp_service/platforms/wedge/WedgeManagerInit.h"

#include "fboss/qsfp_service/platforms/wedge/WedgeManager.h"

namespace facebook {
namespace fboss {
std::unique_ptr<WedgeManager> createFBWedgeManager(
    std::unique_ptr<PlatformProductInfo> /*productInfo*/) {
  return std::unique_ptr<WedgeManager>{};
}

std::unique_ptr<WedgeManager> createYampWedgeManager() {
  return std::unique_ptr<WedgeManager>{};
}

std::unique_ptr<WedgeManager> createDarwinWedgeManager() {
  return std::unique_ptr<WedgeManager>{};
}

std::unique_ptr<WedgeManager> createLassenWedgeManager() {
  return std::unique_ptr<WedgeManager>{};
}

std::unique_ptr<WedgeManager> createElbertWedgeManager() {
  return std::unique_ptr<WedgeManager>{};
}

std::shared_ptr<FbossMacsecHandler> createFbossMacsecHandler(
    WedgeManager* /* wedgeMgr */) {
  return nullptr;
}

bool isElbert8DD() {
  return false;
}

std::unique_ptr<WedgeManager> createSandiaWedgeManager() {
  return std::unique_ptr<WedgeManager>{};
}

std::unique_ptr<WedgeManager> createMakaluWedgeManager() {
  return std::unique_ptr<WedgeManager>{};
}

} // namespace fboss
} // namespace facebook
