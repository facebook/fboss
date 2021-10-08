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
  // TODO - get egress packet stats to obtain packet number
  health.lowestAcceptablePN_ref() = 1;
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
  return health;
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

MacsecStats SaiPhyManager::getMacsecStats(const std::string& portName) const {
  auto saiSwitch = getSaiSwitch(getPortId(portName));
  auto hwPortStats = saiSwitch->getPortStats()[portName];
  return hwPortStats.macsecStats_ref() ? *hwPortStats.macsecStats_ref()
                                       : MacsecStats{};
}

mka::MacsecPortStats SaiPhyManager::getMacsecPortStats(
    std::string portName,
    mka::MacsecDirection direction) {
  return *getMacsecStats(portName).portStats_ref();
}
mka::MacsecFlowStats SaiPhyManager::getMacsecFlowStats(
    std::string portName,
    mka::MacsecDirection direction) {
  auto macsecStats = getMacsecStats(portName);
  return direction == mka::MacsecDirection::INGRESS
      ? *macsecStats.ingressFlowStats_ref()
      : *macsecStats.egressFlowStats_ref();
}

// TODO this API should really require a SCI
mka::MacsecSaStats SaiPhyManager::getMacsecSecureAssocStats(
    std::string portName,
    mka::MacsecDirection direction) {
  auto macsecStats = getMacsecStats(portName);
  // TODO: get active SA from SC
  if (direction == mka::MacsecDirection::INGRESS) {
    return macsecStats.rxSecureAssociationStats_ref()->size()
        ? macsecStats.rxSecureAssociationStats_ref()->begin()->second
        : mka::MacsecSaStats{};
  } else {
    return macsecStats.txSecureAssociationStats_ref()->size()
        ? macsecStats.txSecureAssociationStats_ref()->begin()->second
        : mka::MacsecSaStats{};
  }
}

} // namespace facebook::fboss
