/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/lib/phy/SaiPhyManager.h"

#include "fboss/agent/hw/sai/switch/SaiPortManager.h"
#include "fboss/agent/platforms/sai/SaiHwPlatform.h"
#include "fboss/lib/config/PlatformConfigUtils.h"
#include "fboss/lib/phy/NullPortStats.h"

#include <thrift/lib/cpp/util/EnumUtils.h>

namespace facebook::fboss {
SaiPhyManager::SaiPhyManager(const PlatformMapping* platformMapping)
    : PhyManager(platformMapping) {}

SaiPhyManager::~SaiPhyManager() {}

SaiHwPlatform* SaiPhyManager::getSaiPlatform(GlobalXphyID xphyID) const {
  const auto& phyIDInfo = getPhyIDInfo(xphyID);
  auto pimPlatforms = saiPlatforms_.find(phyIDInfo.pimID);
  if (pimPlatforms == saiPlatforms_.end()) {
    throw FbossError("No SaiHwPlatform is created for pimID:", phyIDInfo.pimID);
  }
  auto platformItr = pimPlatforms->second.find(xphyID);
  if (platformItr == pimPlatforms->second.end()) {
    throw FbossError("SaiHwPlatform is not created for globalPhyID:", xphyID);
  }
  return platformItr->second.get();
}

SaiHwPlatform* SaiPhyManager::getSaiPlatform(PortID portID) const {
  return getSaiPlatform(getGlobalXphyIDbyPortID(portID));
}

SaiSwitch* SaiPhyManager::getSaiSwitch(GlobalXphyID xphyID) const {
  return static_cast<SaiSwitch*>(getSaiPlatform(xphyID)->getHwSwitch());
}
SaiSwitch* SaiPhyManager::getSaiSwitch(PortID portID) const {
  return static_cast<SaiSwitch*>(getSaiPlatform(portID)->getHwSwitch());
}

void SaiPhyManager::addSaiPlatform(
    GlobalXphyID xphyID,
    std::unique_ptr<SaiHwPlatform> platform) {
  const auto phyIDInfo = getPhyIDInfo(xphyID);
  saiPlatforms_[phyIDInfo.pimID][xphyID] = std::move(platform);
}

void SaiPhyManager::sakInstallTx(const mka::MKASak& sak) {
  auto portId = getPortId(*sak.l2Port_ref());
  auto macsecManager = getMacsecManager(portId);
  macsecManager->setupMacsec(
      portId, sak, *sak.sci_ref(), SAI_MACSEC_DIRECTION_EGRESS);
}

void SaiPhyManager::sakInstallRx(
    const mka::MKASak& sak,
    const mka::MKASci& sci) {
  auto portId = getPortId(*sak.l2Port_ref());
  auto macsecManager = getMacsecManager(portId);
  // Use the SCI of the peer
  macsecManager->setupMacsec(portId, sak, sci, SAI_MACSEC_DIRECTION_INGRESS);
}

void SaiPhyManager::sakDeleteRx(
    const mka::MKASak& sak,
    const mka::MKASci& sci) {
  auto portId = getPortId(*sak.l2Port_ref());
  auto macsecManager = getMacsecManager(portId);
  // Use the SCI of the peer
  macsecManager->deleteMacsec(portId, sak, sci, SAI_MACSEC_DIRECTION_INGRESS);
}

void SaiPhyManager::sakDelete(const mka::MKASak& sak) {
  auto portId = getPortId(*sak.l2Port_ref());
  auto macsecManager = getMacsecManager(portId);
  macsecManager->deleteMacsec(
      portId, sak, *sak.sci_ref(), SAI_MACSEC_DIRECTION_EGRESS);
}

SaiMacsecManager* SaiPhyManager::getMacsecManager(PortID portId) {
  auto saiSwitch = getSaiSwitch(portId);
  return &saiSwitch->managerTable()->macsecManager();
}

PortID SaiPhyManager::getPortId(std::string portName) {
  auto platPorts = getPlatformMapping()->getPlatformPorts();
  for (const auto& pair : platPorts) {
    if (folly::to<std::string>(*pair.second.mapping_ref()->name_ref()) ==
        portName) {
      return PortID(pair.first);
    }
  }
  throw FbossError("Could not find port ID for port ", portName);
}

/*
 * programOnePort
 *
 * This function programs one port in the PHY. The port and profile Id is
 * passed to SaiPortManager which in turn uses platform mapping to get the
 * system and line side lane list, speed, fec values. The SaiPortManager
 * will create system port, line port, port connector and return the Sai
 * object for line port. The called function also enables the sys/line port
 */
void SaiPhyManager::programOnePort(
    PortID portId,
    cfg::PortProfileID portProfileId,
    std::optional<TransceiverInfo> transceiverInfo) {
  const auto& wLockedCache = getWLockedCache(portId);

  // Get phy platform
  auto globalPhyID = getGlobalXphyIDbyPortIDLocked(wLockedCache);
  auto saiPlatform = getSaiPlatform(globalPhyID);
  auto saiSwitch = static_cast<SaiSwitch*>(saiPlatform->getHwSwitch());
  const auto& desiredPhyPortConfig =
      getDesiredPhyPortConfig(portId, portProfileId, transceiverInfo);

  // Before actually calling sai sdk to program the port again, we should check
  // whether the port has been programmed in HW with the same config.
  if (!(wLockedCache->systemLanes.empty() || wLockedCache->lineLanes.empty())) {
    const auto& actualPhyPortConfig =
        getHwPhyPortConfigLocked(wLockedCache, portId);
    if (actualPhyPortConfig == desiredPhyPortConfig) {
      XLOG(DBG2) << "External phy config match. Skip reprogramming for port:"
                 << portId << " with profile:"
                 << apache::thrift::util::enumNameSafe(portProfileId);
      return;
    } else {
      XLOG(DBG2) << "External phy config mismatch. Port:" << portId
                 << " with profile:"
                 << apache::thrift::util::enumNameSafe(portProfileId)
                 << " current config:" << actualPhyPortConfig.toDynamic()
                 << ", desired config:" << desiredPhyPortConfig.toDynamic();
    }
  }

  // Temporary object to pass port and profile id to SaiPortManager
  auto portObj = std::make_shared<Port>(
      PortID(portId),
      saiPlatform->getPlatformPort(PortID(portId))
          ->getPlatformPortEntry()
          .mapping_ref()
          ->name_ref()
          .value());
  portObj->setProfileId(portProfileId);
  portObj->setAdminState(cfg::PortState::ENABLED);

  // Finally adding the port (sysport, lineport and port connector).
  // Return value is line port
  PortSaiId saiPort = saiSwitch->managerTable()->portManager().addPort(portObj);
  XLOG(INFO) << "Created Sai port " << saiPort << " for id=" << portId;

  // Once the port is programmed successfully, update the portToLanesInfo_
  setPortToLanesInfoLocked(wLockedCache, portId, desiredPhyPortConfig);
  if (getExternalPhyLocked(wLockedCache)
          ->isSupported(phy::ExternalPhy::Feature::PORT_STATS)) {
    setPortToExternalPhyPortStatsLocked(
        wLockedCache, createExternalPhyPortStats(PortID(portId)));
  }
}

/*
 * macsecGetPhyLinkInfo
 *
 * Get the macsec phy line side link information from SaiPortManager
 * SaiPortHandle attribute
 */
PortOperState SaiPhyManager::macsecGetPhyLinkInfo(PortID swPort) {
  // Get phy platform
  auto globalPhyID = getGlobalXphyIDbyPortID(swPort);
  auto saiPlatform = getSaiPlatform(globalPhyID);
  auto saiSwitch = static_cast<SaiSwitch*>(saiPlatform->getHwSwitch());

  // Get port handle and then get port attribute for oper state
  auto portHandle =
      saiSwitch->managerTable()->portManager().getPortHandle(swPort);

  if (portHandle == nullptr) {
    throw FbossError(folly::sformat(
        "PortHandle not found for port {}", static_cast<int>(swPort)));
  }

  auto portOperStatus = SaiApiTable::getInstance()->portApi().getAttribute(
      portHandle->port->adapterKey(), SaiPortTraits::Attributes::OperStatus{});

  return (portOperStatus == SAI_PORT_OPER_STATUS_UP) ? PortOperState::UP
                                                     : PortOperState::DOWN;
}

std::unique_ptr<ExternalPhyPortStatsUtils>
SaiPhyManager::createExternalPhyPortStats(PortID portID) {
  // TODO(joseph5wu) Need to check what kinda stas we can get from
  // SaiPhyManager here
  return std::make_unique<NullPortStats>(getPortName(portID));
}

} // namespace facebook::fboss
