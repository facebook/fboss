// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/lib/phy/PhyManager.h"

#include "fboss/agent/platforms/common/PlatformMapping.h"
#include "fboss/agent/types.h"
#include "fboss/lib/config/PlatformConfigUtils.h"

#include <folly/logging/xlog.h>
#include "fboss/agent/FbossError.h"

namespace facebook::fboss {
PhyManager::PhyManager(const PlatformMapping* platformMapping) {
  const auto& chips = platformMapping->getChips();
  for (const auto& port : platformMapping->getPlatformPorts()) {
    const auto& xphy = utility::getDataPlanePhyChips(
        port.second, chips, phy::DataPlanePhyChipType::XPHY);
    if (!xphy.empty()) {
      portToGlobalXphyID_.emplace(
          PortID(port.first),
          GlobalXphyID(*xphy.begin()->second.physicalID_ref()));
    }
  }
}

GlobalXphyID PhyManager::getGlobalXphyIDbyPortID(PortID portID) const {
  if (auto id = portToGlobalXphyID_.find(portID);
      id != portToGlobalXphyID_.end()) {
    return id->second;
  }
  throw FbossError(
      "Unable to get GlobalXphyID for port:",
      portID,
      ", current portToGlobalXphyID_ size:",
      portToGlobalXphyID_.size());
}

phy::ExternalPhy* PhyManager::getExternalPhy(GlobalXphyID xphyID) {
  auto pimID = getPhyIDInfo(xphyID).pimID;
  const auto& pimXphyMap = xphyMap_.find(pimID);
  if (pimXphyMap == xphyMap_.end()) {
    throw FbossError(
        "getExternalPhy: Invalid Slot Id ", pimID, " is not in xphyMap_");
  }
  if (pimXphyMap->second.find(xphyID) == pimXphyMap->second.end()) {
    throw FbossError(
        "getExternalPhy: Invalid Global Xphy Id ",
        xphyID,
        " is not in xphyMap_ for Pim Id ",
        pimID);
  }
  // Return the externalPhy object for this slot, mdio, phy
  return xphyMap_[pimID][xphyID].get();
}

/*
 * getExternalPhy
 *
 * This function returns the ExternalPhy object for the given slot number,
 * mdio controller id and phy id.
 */
phy::ExternalPhy*
PhyManager::getExternalPhy(int slotId, int mdioId, int phyId) {
  // Check if the slot id is valid
  if (slotId >= numOfSlot_) {
    throw FbossError(folly::sformat(
        "getExternalPhy: Invalid Slot Id {:d} >= {:d}", slotId, numOfSlot_));
  }
  if (externalPhyMap_.find(slotId) == externalPhyMap_.end()) {
    throw FbossError(
        "getExternalPhy: Invalid Slot Id ",
        slotId,
        " is not in externalPhyMap_");
  }
  if (numMdioController_.find(slotId) == numMdioController_.end()) {
    throw FbossError(
        "getExternalPhy: Invalid Slot Id ",
        slotId,
        " is not in numMdioController_");
  }
  // Check if within the slot, the MDIO based phy list is populated
  if (mdioId >= numMdioController_[slotId] ||
      mdioId >= externalPhyMap_[slotId].size()) {
    throw FbossError(folly::sformat(
        "getExternalPhy: Invalid Mdio Id {:d}, numMdioController_ is {:d}, "
        "externalPhyMap_[{:d}] size is {:d}",
        mdioId,
        numMdioController_[slotId],
        slotId,
        externalPhyMap_[slotId].size()));
  }
  // Check if within the slot and mdio id, the phy object for the phy id exist
  if (phyId >= externalPhyMap_[slotId][mdioId].size()) {
    throw FbossError(folly::sformat(
        "getExternalPhy: Invalid Phy Id {:d}, "
        "externalPhyMap_[{:d}][{:d}] size is {:d}",
        phyId,
        slotId,
        mdioId,
        externalPhyMap_[slotId][mdioId].size()));
  }
  // Return the externalPhy object for this slot, mdio, phy
  return externalPhyMap_[slotId][mdioId][phyId].get();
}

/*
 * programOnePort
 *
 * Calls the ExternalPhy programOnePort function. This function will be called
 * by application like agent.
 */
void PhyManager::programOnePort(
    int slotId,
    int mdioId,
    int phyId,
    phy::PhyPortConfig config) {
  auto xphy = getExternalPhy(slotId, mdioId, phyId);
  // Call the ExternalPhy function
  xphy->programOnePort(config);
};

/*
 * setPortPrbs
 *
 * Calls the ExternalPhy setPortPrbs function. This function will be called
 * by application like agent.
 */
void PhyManager::setPortPrbs(
    int slotId,
    int mdioId,
    int phyId,
    phy::PhyPortConfig config,
    phy::Side side,
    bool enable,
    int32_t polynominal) {
  auto xphy = getExternalPhy(slotId, mdioId, phyId);
  // Call the ExternalPhy function
  xphy->setPortPrbs(config, side, enable, polynominal);
};

/*
 * getPortStats
 *
 * Calls the ExternalPhy getPortStats function. This function will be called
 * by application like agent.
 */
phy::ExternalPhyPortStats PhyManager::getPortStats(
    int slotId,
    int mdioId,
    int phyId,
    phy::PhyPortConfig config) {
  auto xphy = getExternalPhy(slotId, mdioId, phyId);
  // Call the ExternalPhy function
  return xphy->getPortStats(config);
};

/*
 * getPortPrbsStats
 *
 * Calls the ExternalPhy getPortPrbsStats function. This function will be
 * called by application like agent.
 */
phy::ExternalPhyPortStats PhyManager::getPortPrbsStats(
    int slotId,
    int mdioId,
    int phyId,
    phy::PhyPortConfig config) {
  auto xphy = getExternalPhy(slotId, mdioId, phyId);
  // Call the ExternalPhy function
  return xphy->getPortPrbsStats(config);
};

/*
 * getLaneSpeed
 *
 * Calls the ExternalPhy getLaneSpeed function. This function will be called
 * by application like agent.
 */
float_t PhyManager::getLaneSpeed(
    int slotId,
    int mdioId,
    int phyId,
    phy::PhyPortConfig config,
    phy::Side side) {
  auto xphy = getExternalPhy(slotId, mdioId, phyId);
  // Call the ExternalPhy function
  return xphy->getLaneSpeed(config, side);
};

GlobalXphyID PhyManager::getGlobalXphyID(
    const phy::PhyIDInfo& /* phyIDInfo */) const {
  // TODO(joseph5wu) Will make it pure virtual once we have all PhyManager
  // child classes to implement their own version.
  throw FbossError("getGlobalXphyID() is unsupported for such Phymanager");
}

} // namespace facebook::fboss
