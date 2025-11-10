/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/lib/phy/BspSaiPhyManager.h"

#include <folly/logging/xlog.h>
#include "fboss/agent/FbossError.h"

namespace facebook::fboss {

BspSaiPhyManager::BspSaiPhyManager(
    const PlatformMapping* platformMapping,
    const BspSystemContainer* systemContainer)
    : SaiPhyManager(platformMapping), systemContainer_(systemContainer) {
  // Store BSP mapping for later use
  bspMapping_ = std::make_unique<BspPlatformMapping>(
      *systemContainer->getBspPlatformMapping());

  // Set threading model to XPHY_LEVEL for per-xphy threading
  setXphyThreadingModel(XphyThreadingModel::XPHY_LEVEL);

  // Set number of slots from BSP mapping
  numOfSlot_ = systemContainer->getNumPims();

  XLOG(INFO) << "BspSaiPhyManager initialized with " << numOfSlot_
             << " PIMs using XPHY_LEVEL threading model";
}

BspSaiPhyManager::~BspSaiPhyManager() {}

phy::PhyIDInfo BspSaiPhyManager::getPhyIDInfo(GlobalXphyID xphyID) const {
  // For BSP based systems, we are defining GlobalXphyID as the phyID directly
  // Search through mapping to find which PIM this phy belongs to
  for (const auto& [pimID, pimMapping] : bspMapping_->getPimMappings()) {
    if (pimMapping.phyMapping()->find(xphyID) !=
        pimMapping.phyMapping()->end()) {
      const auto& phyMapping = pimMapping.phyMapping()->at(xphyID);
      phy::PhyIDInfo info;
      info.pimID = pimID;
      info.controllerID = *phyMapping.phyIOControllerId();
      info.phyAddr = *phyMapping.phyAddr();
      return info;
    }
  }
  throw FbossError("Cannot find PhyIDInfo for GlobalXphyID=", xphyID);
}

GlobalXphyID BspSaiPhyManager::getGlobalXphyID(
    const phy::PhyIDInfo& phyIDInfo) const {
  // Search through mapping to find the phyID
  const auto& pimMappings = bspMapping_->getPimMappings();
  auto pimIt = pimMappings.find(phyIDInfo.pimID);
  if (pimIt == pimMappings.end()) {
    throw FbossError("Cannot find PIM=", phyIDInfo.pimID, " in BSP mapping");
  }

  // Find the phyID that matches the controllerID and phyAddr
  for (const auto& [phyID, phyMapping] : *pimIt->second.phyMapping()) {
    if (MdioControllerID(*phyMapping.phyIOControllerId()) ==
            phyIDInfo.controllerID &&
        PhyAddr(*phyMapping.phyAddr()) == phyIDInfo.phyAddr) {
      return GlobalXphyID(phyID);
    }
  }

  throw FbossError(
      "Cannot find GlobalXphyID for PIM=",
      phyIDInfo.pimID,
      " controllerID=",
      phyIDInfo.controllerID,
      " phyAddr=",
      phyIDInfo.phyAddr);
}

bool BspSaiPhyManager::initExternalPhyMap(bool /* warmboot */) {
  throw FbossError(
      "BspSaiPhyManager::initExternalPhyMap() not yet implemented");
}

void BspSaiPhyManager::initializeXphy(
    GlobalXphyID /* xphyID */,
    bool /* warmboot */) {
  throw FbossError("BspSaiPhyManager::initializeXphy() not yet implemented");
}

void BspSaiPhyManager::initializeSlotPhys(PimID pimID, bool /* warmboot */) {
  // Hard fail - BspSaiPhyManager uses XPHY_LEVEL threading only
  throw FbossError(
      "initializeSlotPhys() should not be called for BspSaiPhyManager. "
      "This platform uses XPHY_LEVEL threading model. "
      "Use initializeXphy() instead. PimID=",
      pimID);
}

void BspSaiPhyManager::createExternalPhy(
    const phy::PhyIDInfo& /* phyIDInfo */,
    MultiPimPlatformPimContainer* /* pimContainer */) {
  throw FbossError("BspSaiPhyManager::createExternalPhy() not yet implemented");
}

MultiPimPlatformSystemContainer* BspSaiPhyManager::getSystemContainer() {
  throw FbossError(
      "BspSaiPhyManager::getSystemContainer() not implemented. "
      "BSP system container should be passed to constructor.");
}

int BspSaiPhyManager::getPimStartNum() {
  return systemContainer_->getPimStartNum();
}

} // namespace facebook::fboss
