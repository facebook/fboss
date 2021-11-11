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
  auto wCurState = appliedStateDontUseDirectly_.wlock();
  *wCurState = newState;
  (*wCurState)->publish();
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
  auto saiPlatform = getSaiPlatform(portId);
  auto updateFn = [saiPlatform, portId, this, &sak](
                      std::shared_ptr<SwitchState> in) {
    return portUpdateHelper(
        in, portId, saiPlatform, [&sak](auto& port) { port->setTxSak(sak); });
  };
  getPlatformInfo(portId)->applyUpdate(
      folly::sformat("Add SAK tx for : {}", *sak.l2Port_ref()), updateFn);
}

void SaiPhyManager::sakInstallRx(
    const mka::MKASak& sak,
    const mka::MKASci& sci) {
  auto portId = getPortId(*sak.l2Port_ref());
  auto saiPlatform = getSaiPlatform(portId);
  auto updateFn = [saiPlatform, portId, this, &sak, &sci](
                      std::shared_ptr<SwitchState> in) {
    return portUpdateHelper(in, portId, saiPlatform, [&sak, &sci](auto& port) {
      auto rxSaks = port->getRxSaks();
      PortFields::MKASakKey key{sci, *sak.assocNum_ref()};
      rxSaks.erase(key);
      rxSaks.emplace(std::make_pair(key, sak));
      port->setRxSaks(rxSaks);
    });
  };
  getPlatformInfo(portId)->applyUpdate(
      folly::sformat("Add SAK rx for : {}", *sak.l2Port_ref()), updateFn);
}

void SaiPhyManager::sakDeleteRx(
    const mka::MKASak& sak,
    const mka::MKASci& sci) {
  auto portId = getPortId(*sak.l2Port_ref());
  auto saiPlatform = getSaiPlatform(portId);
  auto updateFn = [saiPlatform, portId, this, &sak, &sci](
                      std::shared_ptr<SwitchState> in) {
    return portUpdateHelper(in, portId, saiPlatform, [&sak, &sci](auto& port) {
      auto rxSaks = port->getRxSaks();
      PortFields::MKASakKey key{sci, *sak.assocNum_ref()};
      rxSaks.erase(key);
      port->setRxSaks(rxSaks);
    });
  };
  getPlatformInfo(portId)->applyUpdate(
      folly::sformat("Delete SAK rx for : {}", *sak.l2Port_ref()), updateFn);
}

void SaiPhyManager::sakDelete(const mka::MKASak& sak) {
  auto portId = getPortId(*sak.l2Port_ref());
  auto saiPlatform = getSaiPlatform(portId);
  auto updateFn = [portId, saiPlatform, this](std::shared_ptr<SwitchState> in) {
    return portUpdateHelper(in, portId, saiPlatform, [](auto& port) {
      port->setTxSak(std::nullopt);
    });
  };
  getPlatformInfo(portId)->applyUpdate(
      folly::sformat("Delete SAK tx for : {}", *sak.l2Port_ref()), updateFn);
}

mka::MKASakHealthResponse SaiPhyManager::sakHealthCheck(
    const mka::MKASak sak) const {
  auto portId = getPortId(*sak.l2Port_ref());
  mka::MKASakHealthResponse health;
  health.active_ref() = false;
  bool rxActive{false}, txActive{false};
  auto switchState = getPlatformInfo(portId)->getState();
  auto port = switchState->getPorts()->getPortIf(portId);
  if (port) {
    txActive = port->getTxSak() && *port->getTxSak() == sak;
    for (const auto& keyAndSak : port->getRxSaks()) {
      if (keyAndSak.second == sak) {
        rxActive = true;
        break;
      }
    }
  }
  health.active_ref() = txActive && rxActive;
  health.lowestAcceptablePN_ref() = 1;
  if (*health.active_ref()) {
    auto macsecStats = getMacsecStats(*sak.l2Port_ref());
    mka::MKASecureAssociationId saId;
    saId.sci_ref() = *sak.sci_ref();
    saId.assocNum_ref() = *sak.assocNum_ref();
    // Find the saStats for this sa
    for (auto& txSaStats : macsecStats.txSecureAssociationStats_ref().value()) {
      if (txSaStats.saId_ref()->sci_ref().value() == sak.sci_ref().value() &&
          txSaStats.saId_ref()->assocNum_ref().value() ==
              sak.assocNum_ref().value()) {
        health.lowestAcceptablePN_ref() =
            txSaStats.saStats_ref()->outEncryptedPkts_ref().value();
        break;
      }
    }
  }
  return health;
}

PortID SaiPhyManager::getPortId(std::string portName) const {
  auto portIdVal = folly::tryTo<int>(portName);
  if (portIdVal.hasValue()) {
    return PortID(portIdVal.value());
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

std::shared_ptr<SwitchState> SaiPhyManager::portUpdateHelper(
    std::shared_ptr<SwitchState> in,
    PortID portId,
    const SaiHwPlatform* saiPlatform,
    const std::function<void(std::shared_ptr<Port>&)>& modify) const {
  auto newState = in->clone();
  auto newPorts = newState->getPorts()->modify(&newState);
  // Lookup or create SwitchState port
  auto portObj = newPorts->getPortIf(portId);
  portObj = portObj
      ? portObj->clone()
      : std::make_shared<Port>(PortID(portId), getPortName(portId));
  // Modify port fields
  modify(portObj);
  if (newPorts->getPortIf(portId)) {
    newPorts->updatePort(std::move(portObj));
  } else {
    newPorts->addPort(std::move(portObj));
  }
  return newState;
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

  // Before actually calling sai sdk to program the port again, we should
  // check whether the port has been programmed in HW with the same config.
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

  auto updateFn =
      [this, &saiPlatform, portId, portProfileId, desiredPhyPortConfig](
          std::shared_ptr<SwitchState> in) {
        return portUpdateHelper(
            in,
            portId,
            saiPlatform,
            [portProfileId, desiredPhyPortConfig](auto& port) {
              port->setSpeed(desiredPhyPortConfig.profile.speed);
              port->setProfileId(portProfileId);
              port->setAdminState(cfg::PortState::ENABLED);
              // Prepare the side profileConfig and pinConfigs for both system
              // and line sides by using desiredPhyPortConfig
              port->setProfileConfig(desiredPhyPortConfig.profile.system);
              port->resetPinConfigs(
                  desiredPhyPortConfig.config.system.getPinConfigs());
              port->setLineProfileConfig(desiredPhyPortConfig.profile.line);
              port->resetLinePinConfigs(
                  desiredPhyPortConfig.config.line.getPinConfigs());
            });
      };
  getPlatformInfo(globalPhyID)
      ->applyUpdate(
          folly::sformat("Port {} program", static_cast<int>(portId)),
          updateFn);

  // Once the port is programmed successfully, update the portToCacheInfo_
  setPortToPortCacheInfoLocked(
      wLockedCache, portId, portProfileId, desiredPhyPortConfig);
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

MacsecStats SaiPhyManager::getMacsecStatsFromHw(const std::string& portName) {
  auto saiSwitch = getSaiSwitch(getPortId(portName));
  static SwitchStats unused;
  saiSwitch->updateStats(&unused);
  return getMacsecStats(portName);
}

MacsecStats SaiPhyManager::getMacsecStats(const std::string& portName) const {
  auto portId = getPortId(portName);
  auto saiSwitch = getSaiSwitch(portId);
  auto pName =
      folly::tryTo<int>(portName).hasValue() ? getPortName(portId) : portName;
  auto hwPortStats = saiSwitch->getPortStats()[pName];
  return hwPortStats.macsecStats_ref() ? *hwPortStats.macsecStats_ref()
                                       : MacsecStats{};
}

/*
 * getAllMacsecPortStats
 *
 * Get the macsec stats for all ports in the system. This function will loop
 * through all pim platforms and then all xphy in the pim platform and then get
 * macsec stats for all ports. This function returns the map of port name to
 * MacsecStats structure which contains port, flow and SA stats
 */
std::map<std::string, MacsecStats> SaiPhyManager::getAllMacsecPortStats(
    bool readFromHw) {
  std::map<std::string, MacsecStats> phyPortStatsMap;

  // Loop through all pim platforms
  for (auto& pimPlatformItr : saiPlatforms_) {
    auto& pimPlatform = pimPlatformItr.second;

    // Loop through all xphy in the pim
    for (auto& platformItr : pimPlatform) {
      GlobalXphyID xphyID = platformItr.first;

      // Get SaiSwitch using global xphy id
      auto saiSwitch = getSaiSwitch(xphyID);

      if (readFromHw) {
        static SwitchStats unused;
        saiSwitch->updateStats(&unused);
      }

      // Call getPortStats for the particular Phy and fill in to return map
      auto xphyPortStats = saiSwitch->getPortStats();
      for (auto& statsItr : xphyPortStats) {
        phyPortStatsMap[statsItr.first] =
            statsItr.second.macsecStats_ref().has_value()
            ? statsItr.second.macsecStats_ref().value()
            : MacsecStats{};
      }
    }
  }
  return phyPortStatsMap;
}

/*
 * getMacsecPortStats
 *
 * Get the macsec stats for some ports in the system
 */
std::map<std::string, MacsecStats> SaiPhyManager::getMacsecPortStats(
    const std::vector<std::string>& portNames,
    bool readFromHw) {
  std::map<std::string, MacsecStats> phyPortStatsMap;

  for (auto& portName : portNames) {
    phyPortStatsMap[portName] = getMacsecStats(portName, readFromHw);
  }
  return phyPortStatsMap;
}

mka::MacsecPortStats SaiPhyManager::getMacsecPortStats(
    std::string portName,
    mka::MacsecDirection direction,
    bool readFromHw) {
  auto macsecStats = getMacsecStats(portName, readFromHw);
  return direction == mka::MacsecDirection::INGRESS
      ? *macsecStats.ingressPortStats_ref()
      : *macsecStats.egressPortStats_ref();
}

mka::MacsecFlowStats SaiPhyManager::getMacsecFlowStats(
    std::string portName,
    mka::MacsecDirection direction,
    bool readFromHw) {
  auto macsecStats = getMacsecStats(portName, readFromHw);

  // Consolidate the stats from all the SCI
  auto sumFlowStats = [](std::vector<MacsecSciFlowStats>& sciFlowStats)
      -> mka::MacsecFlowStats {
    mka::MacsecFlowStats returnFlowStats{};
    for (auto& singleFlowStat : sciFlowStats) {
      returnFlowStats.directionIngress_ref() =
          singleFlowStat.flowStats_ref()->directionIngress_ref().value();
      returnFlowStats.ucastUncontrolledPkts_ref() =
          returnFlowStats.ucastUncontrolledPkts_ref().value() +
          singleFlowStat.flowStats_ref()->ucastUncontrolledPkts_ref().value();
      returnFlowStats.ucastControlledPkts_ref() =
          returnFlowStats.ucastControlledPkts_ref().value() +
          singleFlowStat.flowStats_ref()->ucastControlledPkts_ref().value();
      returnFlowStats.mcastUncontrolledPkts_ref() =
          returnFlowStats.mcastUncontrolledPkts_ref().value() +
          singleFlowStat.flowStats_ref()->mcastUncontrolledPkts_ref().value();
      returnFlowStats.mcastControlledPkts_ref() =
          returnFlowStats.mcastControlledPkts_ref().value() +
          singleFlowStat.flowStats_ref()->mcastControlledPkts_ref().value();
      returnFlowStats.bcastUncontrolledPkts_ref() =
          returnFlowStats.bcastUncontrolledPkts_ref().value() +
          singleFlowStat.flowStats_ref()->bcastUncontrolledPkts_ref().value();
      returnFlowStats.bcastControlledPkts_ref() =
          returnFlowStats.bcastControlledPkts_ref().value() +
          singleFlowStat.flowStats_ref()->bcastControlledPkts_ref().value();
      returnFlowStats.controlPkts_ref() =
          returnFlowStats.controlPkts_ref().value() +
          singleFlowStat.flowStats_ref()->controlPkts_ref().value();
      returnFlowStats.untaggedPkts_ref() =
          returnFlowStats.untaggedPkts_ref().value() +
          singleFlowStat.flowStats_ref()->untaggedPkts_ref().value();
      returnFlowStats.otherErrPkts_ref() =
          returnFlowStats.otherErrPkts_ref().value() +
          singleFlowStat.flowStats_ref()->otherErrPkts_ref().value();
      returnFlowStats.octetsUncontrolled_ref() =
          returnFlowStats.octetsUncontrolled_ref().value() +
          singleFlowStat.flowStats_ref()->octetsUncontrolled_ref().value();
      returnFlowStats.octetsControlled_ref() =
          returnFlowStats.octetsControlled_ref().value() +
          singleFlowStat.flowStats_ref()->octetsControlled_ref().value();
      returnFlowStats.outCommonOctets_ref() =
          returnFlowStats.outCommonOctets_ref().value() +
          singleFlowStat.flowStats_ref()->outCommonOctets_ref().value();
      returnFlowStats.outTooLongPkts_ref() =
          returnFlowStats.outTooLongPkts_ref().value() +
          singleFlowStat.flowStats_ref()->outTooLongPkts_ref().value();
      returnFlowStats.inTaggedControlledPkts_ref() =
          returnFlowStats.inTaggedControlledPkts_ref().value() +
          singleFlowStat.flowStats_ref()->inTaggedControlledPkts_ref().value();
      returnFlowStats.inNoTagPkts_ref() =
          returnFlowStats.inNoTagPkts_ref().value() +
          singleFlowStat.flowStats_ref()->inNoTagPkts_ref().value();
      returnFlowStats.inBadTagPkts_ref() =
          returnFlowStats.inBadTagPkts_ref().value() +
          singleFlowStat.flowStats_ref()->inBadTagPkts_ref().value();
      returnFlowStats.noSciPkts_ref() =
          returnFlowStats.noSciPkts_ref().value() +
          singleFlowStat.flowStats_ref()->noSciPkts_ref().value();
      returnFlowStats.unknownSciPkts_ref() =
          returnFlowStats.unknownSciPkts_ref().value() +
          singleFlowStat.flowStats_ref()->unknownSciPkts_ref().value();
      returnFlowStats.overrunPkts_ref() =
          returnFlowStats.overrunPkts_ref().value() +
          singleFlowStat.flowStats_ref()->overrunPkts_ref().value();
    }
    return returnFlowStats;
  };

  if (direction == mka::MacsecDirection::INGRESS) {
    return sumFlowStats(macsecStats.ingressFlowStats_ref().value());
  } else {
    return sumFlowStats(macsecStats.egressFlowStats_ref().value());
  }
}

mka::MacsecSaStats SaiPhyManager::getMacsecSecureAssocStats(
    std::string portName,
    mka::MacsecDirection direction,
    bool readFromHw) {
  auto macsecStats = getMacsecStats(portName, readFromHw);

  // Conslidate stats across all SA
  auto sumSaStats =
      [](std::vector<MacsecSaIdSaStats>& saIdSaStats) -> mka::MacsecSaStats {
    mka::MacsecSaStats returnSaStats{};
    for (auto& singleSaStat : saIdSaStats) {
      returnSaStats.directionIngress_ref() =
          singleSaStat.saStats_ref()->directionIngress_ref().value();
      returnSaStats.octetsEncrypted_ref() =
          returnSaStats.octetsEncrypted_ref().value() +
          singleSaStat.saStats_ref()->octetsEncrypted_ref().value();
      returnSaStats.octetsProtected_ref() =
          returnSaStats.octetsProtected_ref().value() +
          singleSaStat.saStats_ref()->octetsProtected_ref().value();
      returnSaStats.outEncryptedPkts_ref() =
          returnSaStats.outEncryptedPkts_ref().value() +
          singleSaStat.saStats_ref()->outEncryptedPkts_ref().value();
      returnSaStats.outProtectedPkts_ref() =
          returnSaStats.outProtectedPkts_ref().value() +
          singleSaStat.saStats_ref()->outProtectedPkts_ref().value();
      returnSaStats.inUncheckedPkts_ref() =
          returnSaStats.inUncheckedPkts_ref().value() +
          singleSaStat.saStats_ref()->inUncheckedPkts_ref().value();
      returnSaStats.inDelayedPkts_ref() =
          returnSaStats.inDelayedPkts_ref().value() +
          singleSaStat.saStats_ref()->inDelayedPkts_ref().value();
      returnSaStats.inLatePkts_ref() = returnSaStats.inLatePkts_ref().value() +
          singleSaStat.saStats_ref()->inLatePkts_ref().value();
      returnSaStats.inInvalidPkts_ref() =
          returnSaStats.inInvalidPkts_ref().value() +
          singleSaStat.saStats_ref()->inInvalidPkts_ref().value();
      returnSaStats.inNotValidPkts_ref() =
          returnSaStats.inNotValidPkts_ref().value() +
          singleSaStat.saStats_ref()->inNotValidPkts_ref().value();
      returnSaStats.inNoSaPkts_ref() = returnSaStats.inNoSaPkts_ref().value() +
          singleSaStat.saStats_ref()->inNoSaPkts_ref().value();
      returnSaStats.inUnusedSaPkts_ref() =
          returnSaStats.inUnusedSaPkts_ref().value() +
          singleSaStat.saStats_ref()->inUnusedSaPkts_ref().value();
      returnSaStats.inOkPkts_ref() = returnSaStats.inOkPkts_ref().value() +
          singleSaStat.saStats_ref()->inOkPkts_ref().value();
    }
    return returnSaStats;
  };

  if (direction == mka::MacsecDirection::INGRESS) {
    return sumSaStats(macsecStats.rxSecureAssociationStats_ref().value());
  } else {
    return sumSaStats(macsecStats.txSecureAssociationStats_ref().value());
  }
}

} // namespace facebook::fboss
