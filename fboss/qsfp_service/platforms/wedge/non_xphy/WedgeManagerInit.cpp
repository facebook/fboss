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

namespace facebook::fboss {
std::unique_ptr<WedgeManager> createFBWedgeManager(
    std::unique_ptr<PlatformProductInfo> /*productInfo*/,
    const std::shared_ptr<const PlatformMapping> /* platformMapping */,
    const std::shared_ptr<
        std::unordered_map<TransceiverID, SlotThreadHelper>> /* threads */
) {
  return nullptr;
}
std::unique_ptr<WedgeManager> createYampWedgeManager(
    const std::shared_ptr<const PlatformMapping> /* platformMapping */,
    const std::shared_ptr<
        std::unordered_map<TransceiverID, SlotThreadHelper>> /* threads */) {
  return nullptr;
}

std::unique_ptr<WedgeManager> createDarwinWedgeManager(
    const std::shared_ptr<const PlatformMapping> /* platformMapping */,
    const std::shared_ptr<
        std::unordered_map<TransceiverID, SlotThreadHelper>> /* threads */) {
  return nullptr;
}

std::unique_ptr<WedgeManager> createElbertWedgeManager(
    const std::shared_ptr<const PlatformMapping> /* platformMapping */,
    const std::shared_ptr<std::unordered_map<TransceiverID, SlotThreadHelper>>
    /* threads */) {
  // non_xphy build should not be used for Elbert
  return nullptr;
}

bool isElbert8DD() {
  // non_xphy build should not be used for Elbert
  return false;
}

std::shared_ptr<FbossMacsecHandler> createFbossMacsecHandler(
    WedgeManager* wedgeMgr) {
  // No need macsec for non_xphy platforms
  return nullptr;
}

std::unique_ptr<PhyManager> createPhyManager(
    PlatformType /* mode */,
    const PlatformMapping* /* platformMapping */,
    const WedgeManager* /* wedgeManager */) {
  // No need PhyManager for non_xphy platforms
  return nullptr;
}

} // namespace facebook::fboss
