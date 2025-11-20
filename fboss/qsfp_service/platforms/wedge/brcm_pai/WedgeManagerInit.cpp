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

#include "fboss/lib/phy/BspSaiPhyManager.h"
#include "fboss/qsfp_service/platforms/wedge/BspWedgeManager.h"
#include "fboss/qsfp_service/platforms/wedge/FbossMacsecHandler.h"

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
    const std::shared_ptr<const PlatformMapping> /* platformMapping */
    ,
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
  // brcm_pai build should not be used for Elbert
  return nullptr;
}

bool isElbert8DD() {
  // brcm_pai build should not be used for Elbert
  return false;
}

std::shared_ptr<FbossMacsecHandler> createFbossMacsecHandler(
    WedgeManager* wedgeMgr) {
  // Use default FbossMacsecHandler
  return std::make_shared<FbossMacsecHandler>(wedgeMgr);
}

std::unique_ptr<PhyManager> createPhyManager(
    PlatformType /* mode */,
    const PlatformMapping* platformMapping,
    const WedgeManager* wedgeManager) {
  const auto* bspWedgeManager =
      static_cast<const BspWedgeManager*>(wedgeManager);
  const auto* systemContainer = bspWedgeManager->getSystemContainer();
  // Check if BSP mapping has xphy information
  const auto& pimMappings =
      systemContainer->getBspPlatformMapping()->getPimMappings();
  bool hasXphy =
      std::find_if(
          pimMappings.cbegin(), pimMappings.cend(), [](const auto& pair) {
            return !pair.second.phyMapping()->empty();
          }) != pimMappings.cend();

  if (hasXphy) {
    XLOG(INFO)
        << "Current BSP WedgeManager has XPHY in BspPlatformMapping, about to create BspPhyManager";
    return std::make_unique<BspSaiPhyManager>(platformMapping, systemContainer);
  }
  return nullptr;
}
} // namespace facebook::fboss
