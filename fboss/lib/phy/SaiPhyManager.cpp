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

#include "fboss/agent/SwitchStats.h"
#include "fboss/agent/Utils.h"
#include "fboss/agent/hw/sai/switch/SaiPortManager.h"
#include "fboss/agent/platforms/sai/SaiHwPlatform.h"
#include "fboss/agent/state/StateUpdate.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/lib/config/PlatformConfigUtils.h"
#include "fboss/lib/phy/NullPortStats.h"
#include "fboss/lib/phy/gen-cpp2/phy_types.h"

#include <thrift/lib/cpp/util/EnumUtils.h>

namespace {
using namespace facebook::fboss;
sai_macsec_direction_t mkaDirectionToSaiDirection(
    facebook::fboss::mka::MacsecDirection direction) {
  return direction == mka::MacsecDirection::INGRESS
      ? SAI_MACSEC_DIRECTION_INGRESS
      : SAI_MACSEC_DIRECTION_EGRESS;
}
} // namespace

namespace facebook::fboss {

SaiPhyManager::SaiPhyManager(const PlatformMapping* platformMapping)
    : PhyManager(platformMapping), localMac_(getLocalMacAddress()) {}

SaiSwitch* SaiPhyManager::PlatformInfo::getHwSwitch() {
  return static_cast<SaiSwitch*>(saiPlatform_->getHwSwitch());
}

void SaiPhyManager::PlatformInfo::setState(
    const std::shared_ptr<SwitchState>& newState) {
  curState_ = newState;
  curState_->publish();
}

void SaiPhyManager::PlatformInfo::applyUpdate(
    folly::StringPiece name,
    StateUpdateFn updateFn) {
  std::lock_guard<std::mutex> g(updateMutex_);
  XLOG(INFO) << " Applying update : " << name;
  auto oldState = getState();
  auto newState = updateFn(oldState);
  if (newState) {
    auto appliedState =
        getHwSwitch()->stateChanged(StateDelta(oldState, newState));
    setState(appliedState);
  }
}

SaiPhyManager::~SaiPhyManager() {}

SaiPhyManager::PlatformInfo::PlatformInfo(
    std::unique_ptr<SaiHwPlatform> platform)
    : saiPlatform_(std::move(platform)) {
  setState(std::make_shared<SwitchState>());
}

SaiPhyManager::PlatformInfo::~PlatformInfo() {}

SaiPhyManager::PlatformInfo* SaiPhyManager::getPlatformInfo(
    GlobalXphyID xphyID) {
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

SaiPhyManager::PlatformInfo* SaiPhyManager::getPlatformInfo(PortID portID) {
  return getPlatformInfo(getGlobalXphyIDbyPortID(portID));
}

SaiHwPlatform* SaiPhyManager::getSaiPlatform(GlobalXphyID xphyID) {
  return getPlatformInfo(xphyID)->getPlatform();
}

SaiHwPlatform* SaiPhyManager::getSaiPlatform(PortID portID) {
  return getSaiPlatform(getGlobalXphyIDbyPortID(portID));
}

SaiSwitch* SaiPhyManager::getSaiSwitch(GlobalXphyID xphyID) {
  return static_cast<SaiSwitch*>(getSaiPlatform(xphyID)->getHwSwitch());
}
SaiSwitch* SaiPhyManager::getSaiSwitch(PortID portID) {
  return static_cast<SaiSwitch*>(getSaiPlatform(portID)->getHwSwitch());
}

void SaiPhyManager::updateAllXphyPortsStats() {
  for (auto& pimAndXphyToPlatforms : saiPlatforms_) {
    auto pimId = pimAndXphyToPlatforms.first;
    auto& ongoingStatsCollection = pim2OngoingStatsCollection_[pimId];
    if (ongoingStatsCollection && !ongoingStatsCollection->isReady()) {
      XLOG(DBG4) << " Sai stats collection for PIM : " << pimId
                 << "is still ongoing";
    } else {
      pim2OngoingStatsCollection_[pimId] =
          folly::via(getPimEventBase(pimId))
              .thenValue([this, pimId](auto&&) {
                auto& xphyToPlatform = saiPlatforms_.find(pimId)->second;
                for (auto& [xphy, platformInfo] : xphyToPlatform) {
                  try {
                    static SwitchStats unused;
                    platformInfo->getHwSwitch()->updateStats(&unused);
                  } catch (const std::exception& e) {
                    XLOG(INFO) << "Stats collection failed on : "
                               << "switch: "
                               << platformInfo->getHwSwitch()->getSwitchId()
                               << " xphy: " << xphy << " error: " << e.what();
                  }
                }
              })
              .delayed(
                  std::chrono::seconds(getXphyPortStatsUpdateIntervalInSec()));
    }
  }
}

void SaiPhyManager::addSaiPlatform(
    GlobalXphyID xphyID,
    std::unique_ptr<SaiHwPlatform> platform) {
  const auto phyIDInfo = getPhyIDInfo(xphyID);
  saiPlatforms_[phyIDInfo.pimID].emplace(std::make_pair(
      xphyID, std::make_unique<PlatformInfo>(std::move(platform))));
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

mka::MKASakHealthResponse SaiPhyManager::sakHealthCheck(
    const mka::MKASak sak) const {
  auto portId = getPortId(*sak.l2Port_ref());
  auto macsecManager = getMacsecManager(portId);
  return macsecManager->sakHealthCheck(portId, sak);
}

SaiMacsecManager* SaiPhyManager::getMacsecManager(PortID portId) {
  auto saiSwitch = getSaiSwitch(portId);
  return &saiSwitch->managerTable()->macsecManager();
}

const SaiMacsecManager* SaiPhyManager::getMacsecManager(PortID portId) const {
  return const_cast<SaiPhyManager*>(this)->getMacsecManager(portId);
}

PortID SaiPhyManager::getPortId(std::string portName) const {
  try {
    return PortID(folly::to<int>(portName));
  } catch (const std::exception& e) {
    XLOG(INFO) << "Unable to convert port: " << portName
               << " to int. Looking up port id";
  }
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

  auto updateFn = [&saiPlatform, portId, portProfileId, desiredPhyPortConfig](
                      std::shared_ptr<SwitchState> in) {
    auto newState = in->clone();
    auto newPorts = newState->getPorts()->modify(&newState);
    // Temporary object to pass port and profile id to SaiPortManager
    auto port = std::make_shared<Port>(
        PortID(portId),
        saiPlatform->getPlatformPort(PortID(portId))
            ->getPlatformPortEntry()
            .mapping_ref()
            ->name_ref()
            .value());
    port->setSpeed(desiredPhyPortConfig.profile.speed);
    port->setProfileId(portProfileId);
    port->setAdminState(cfg::PortState::ENABLED);
    port->setProfileConfig(desiredPhyPortConfig.profile.system);
    port->resetPinConfigs(desiredPhyPortConfig.config.system.getPinConfigs());
    port->setLineProfileConfig(desiredPhyPortConfig.profile.line);
    port->resetLinePinConfigs(desiredPhyPortConfig.config.line.getPinConfigs());
    if (newPorts->getPortIf(portId)) {
      newPorts->updatePort(std::move(port));
    } else {
      newPorts->addPort(std::move(port));
    }
    return newState;
  };
  getPlatformInfo(globalPhyID)
      ->applyUpdate(
          folly::sformat("Port {} program", static_cast<int>(portId)),
          updateFn);

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

mka::MacsecPortStats SaiPhyManager::getMacsecPortStats(
    std::string portName,
    mka::MacsecDirection direction) {
  mka::MacsecPortStats result;
  auto portId = getPortId(portName);
  auto macsecManager = getMacsecManager(portId);
  auto macsecPort = macsecManager->getMacsecPortHandle(
      portId, mkaDirectionToSaiDirection(direction));
  auto stats = macsecPort->port->getStats<SaiMacsecPortTraits>();
  result.preMacsecDropPkts_ref() =
      stats[SAI_MACSEC_PORT_STAT_PRE_MACSEC_DROP_PKTS];
  result.controlPkts_ref() = stats[SAI_MACSEC_PORT_STAT_CONTROL_PKTS];
  result.dataPkts_ref() = stats[SAI_MACSEC_PORT_STAT_DATA_PKTS];
  return result;
}
mka::MacsecFlowStats SaiPhyManager::getMacsecFlowStats(
    std::string portName,
    mka::MacsecDirection direction) {
  mka::MacsecFlowStats result;
  auto portId = getPortId(portName);
  auto macsecManager = getMacsecManager(portId);
  auto macsecPort = macsecManager->getMacsecPortHandle(
      portId, mkaDirectionToSaiDirection(direction));

  auto& secureChannel = macsecPort->secureChannels.begin()->second;
  auto flow = secureChannel->flow;

  // TODO: get active secure channel instead of just using first?

  auto stats = flow->getStats<SaiMacsecFlowTraits>();
  result.directionIngress_ref() = direction == mka::MacsecDirection::INGRESS;
  result.ucastUncontrolledPkts_ref() =
      stats[SAI_MACSEC_FLOW_STAT_UCAST_PKTS_UNCONTROLLED];
  result.ucastControlledPkts_ref() =
      stats[SAI_MACSEC_FLOW_STAT_UCAST_PKTS_CONTROLLED];
  result.mcastUncontrolledPkts_ref() =
      stats[SAI_MACSEC_FLOW_STAT_MULTICAST_PKTS_UNCONTROLLED];
  result.mcastControlledPkts_ref() =
      stats[SAI_MACSEC_FLOW_STAT_MULTICAST_PKTS_CONTROLLED];
  result.bcastUncontrolledPkts_ref() =
      stats[SAI_MACSEC_FLOW_STAT_BROADCAST_PKTS_UNCONTROLLED];
  result.bcastControlledPkts_ref() =
      stats[SAI_MACSEC_FLOW_STAT_BROADCAST_PKTS_CONTROLLED];
  result.controlPkts_ref() = stats[SAI_MACSEC_FLOW_STAT_CONTROL_PKTS];
  result.untaggedPkts_ref() = stats[SAI_MACSEC_FLOW_STAT_PKTS_UNTAGGED];
  result.otherErrPkts_ref() = stats[SAI_MACSEC_FLOW_STAT_OTHER_ERR];
  result.octetsUncontrolled_ref() =
      stats[SAI_MACSEC_FLOW_STAT_OCTETS_UNCONTROLLED];
  result.octetsControlled_ref() = stats[SAI_MACSEC_FLOW_STAT_OCTETS_CONTROLLED];
  result.outCommonOctets_ref() = stats[SAI_MACSEC_FLOW_STAT_OUT_OCTETS_COMMON];
  result.outTooLongPkts_ref() = stats[SAI_MACSEC_FLOW_STAT_OUT_PKTS_TOO_LONG];
  result.inTaggedControlledPkts_ref() =
      stats[SAI_MACSEC_FLOW_STAT_IN_TAGGED_CONTROL_PKTS];
  result.inUntaggedPkts_ref() = stats[SAI_MACSEC_FLOW_STAT_IN_PKTS_NO_TAG];
  result.inBadTagPkts_ref() = stats[SAI_MACSEC_FLOW_STAT_IN_PKTS_BAD_TAG];
  result.noSciPkts_ref() = stats[SAI_MACSEC_FLOW_STAT_IN_PKTS_NO_SCI];
  result.unknownSciPkts_ref() = stats[SAI_MACSEC_FLOW_STAT_IN_PKTS_UNKNOWN_SCI];
  result.overrunPkts_ref() = stats[SAI_MACSEC_FLOW_STAT_IN_PKTS_OVERRUN];
  return result;
}
mka::MacsecSaStats SaiPhyManager::getMacsecSecureAssocStats(
    std::string portName,
    mka::MacsecDirection direction) {
  mka::MacsecSaStats result;
  auto portId = getPortId(portName);
  auto macsecManager = getMacsecManager(portId);
  auto macsecPort = macsecManager->getMacsecPortHandle(
      portId, mkaDirectionToSaiDirection(direction));
  auto& secureChannel = macsecPort->secureChannels.begin()->second;
  auto& secureAssoc = secureChannel->secureAssocs.begin()->second;

  // TODO: get active SA from SC

  auto stats = secureAssoc->getStats<SaiMacsecSATraits>();

  result.directionIngress_ref() = direction == mka::MacsecDirection::INGRESS;
  result.octetsEncrypted_ref() = stats[SAI_MACSEC_SA_STAT_OCTETS_ENCRYPTED];
  result.octetsProtected_ref() = stats[SAI_MACSEC_SA_STAT_OCTETS_PROTECTED];
  result.outEncryptedPkts_ref() = stats[SAI_MACSEC_SA_STAT_OUT_PKTS_ENCRYPTED];
  result.outProtectedPkts_ref() = stats[SAI_MACSEC_SA_STAT_OUT_PKTS_PROTECTED];
  result.inUncheckedPkts_ref() = stats[SAI_MACSEC_SA_STAT_IN_PKTS_UNCHECKED];
  result.inDelayedPkts_ref() = stats[SAI_MACSEC_SA_STAT_IN_PKTS_DELAYED];
  result.inLatePkts_ref() = stats[SAI_MACSEC_SA_STAT_IN_PKTS_LATE];
  result.inInvalidPkts_ref() = stats[SAI_MACSEC_SA_STAT_IN_PKTS_INVALID];
  result.inNotValidPkts_ref() = stats[SAI_MACSEC_SA_STAT_IN_PKTS_NOT_VALID];
  result.inNoSaPkts_ref() = stats[SAI_MACSEC_SA_STAT_IN_PKTS_NOT_USING_SA];
  result.inUnusedSaPkts_ref() = stats[SAI_MACSEC_SA_STAT_IN_PKTS_UNUSED_SA];
  result.inOkPkts_ref() = stats[SAI_MACSEC_SA_STAT_IN_PKTS_OK];
  return result;
}

} // namespace facebook::fboss
