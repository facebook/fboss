// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/lib/phy/PhyManager.h"
#include <folly/logging/xlog.h>

namespace facebook {
namespace fboss {

/*
 * getExternalPhy
 *
 * This function returns the ExternalPhy object for the given slot number,
 * mdio controller id and phy id.
 */
phy::ExternalPhy*
PhyManager::getExternalPhy(int slotId, int mdioId, int phyId) {
  // Check if the slot id is valid
  if (slotId >= numOfSlot_ || slotId >= externalPhyMap_.size()) {
    throw FbossError(
        folly::sformat("getExternalPhy: Invalid Slot Id {:d}", slotId));
  }
  // Check if within the slot, the MDIO based phy list is populated
  if (mdioId >= numMdioController_[slotId] ||
      mdioId >= externalPhyMap_[slotId].size()) {
    throw FbossError(
        folly::sformat("getExternalPhy: Invalid Mdio Id {:d}", mdioId));
  }
  // Check if within the slot and mdio id, the phy object for the phy id exist
  if (phyId >= externalPhyMap_[slotId][mdioId].size()) {
    throw FbossError(
        folly::sformat("getExternalPhy: Invalid Phy Id {:d}", phyId));
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
 * Calls the ExternalPhy getPortPrbsStats function. This function will be called
 * by application like agent.
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

} // namespace fboss
} // namespace facebook
