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
    try {
      auto appliedState =
          getHwSwitch()->stateChanged(StateDelta(oldState, newState));
      setState(appliedState);
    } catch (const std::exception& e) {
      XLOG(FATAL) << "Failed to apply update: " << name
                  << " exception: " << e.what();
    }
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
          folly::via(getPimEventBase(pimId)).thenValue([this, pimId](auto&&) {
            steady_clock::time_point begin = steady_clock::now();
            auto& xphyToPlatform = saiPlatforms_.find(pimId)->second;
            for (auto& [xphy, platformInfo] : xphyToPlatform) {
              try {
                static SwitchStats unused;
                platformInfo->getHwSwitch()->updateStats(&unused);
                auto phyInfos = platformInfo->getHwSwitch()->updateAllPhyInfo();
                for (auto& [portId, phyInfo] : phyInfos) {
                  updateXphyInfo(portId, std::move(phyInfo));
                }
              } catch (const std::exception& e) {
                XLOG(INFO) << "Stats collection failed on : "
                           << "switch: "
                           << platformInfo->getHwSwitch()->getSaiSwitchId()
                           << " xphy: " << xphy << " error: " << e.what();
              }
            }
            XLOG(DBG3) << "Pim " << static_cast<int>(pimId) << ": all "
                       << xphyToPlatform.size() << " xphy stat collection took "
                       << duration_cast<milliseconds>(
                              steady_clock::now() - begin)
                              .count()
                       << "ms";
          });
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
  auto portId = getPortId(*sak.l2Port());
  auto saiPlatform = getSaiPlatform(portId);
  auto updateFn = [saiPlatform, portId, this, &sak](
                      std::shared_ptr<SwitchState> in) {
    return portUpdateHelper(
        in, portId, saiPlatform, [&sak](auto& port) { port->setTxSak(sak); });
  };
  getPlatformInfo(portId)->applyUpdate(
      folly::sformat("Add SAK tx for : {}", *sak.l2Port()), updateFn);
}

void SaiPhyManager::sakInstallRx(
    const mka::MKASak& sak,
    const mka::MKASci& sci) {
  auto portId = getPortId(*sak.l2Port());
  auto saiPlatform = getSaiPlatform(portId);
  auto updateFn = [saiPlatform, portId, this, &sak, &sci](
                      std::shared_ptr<SwitchState> in) {
    return portUpdateHelper(in, portId, saiPlatform, [&sak, &sci](auto& port) {
      auto rxSaks = port->getRxSaks();
      PortFields::MKASakKey key{sci, *sak.assocNum()};
      rxSaks.erase(key);
      rxSaks.emplace(std::make_pair(key, sak));
      port->setRxSaks(rxSaks);
    });
  };
  getPlatformInfo(portId)->applyUpdate(
      folly::sformat("Add SAK rx for : {}", *sak.l2Port()), updateFn);
}

void SaiPhyManager::sakDeleteRx(
    const mka::MKASak& sak,
    const mka::MKASci& sci) {
  auto portId = getPortId(*sak.l2Port());
  auto saiPlatform = getSaiPlatform(portId);
  auto updateFn = [saiPlatform, portId, this, &sak, &sci](
                      std::shared_ptr<SwitchState> in) {
    return portUpdateHelper(in, portId, saiPlatform, [&sak, &sci](auto& port) {
      auto rxSaks = port->getRxSaks();
      PortFields::MKASakKey key{sci, *sak.assocNum()};
      auto it = rxSaks.find(key);
      if (it != rxSaks.end() && it->second == sak) {
        rxSaks.erase(key);
        port->setRxSaks(rxSaks);
      }
    });
  };
  getPlatformInfo(portId)->applyUpdate(
      folly::sformat("Delete SAK rx for : {}", *sak.l2Port()), updateFn);
}

void SaiPhyManager::sakDelete(const mka::MKASak& sak) {
  auto portId = getPortId(*sak.l2Port());
  auto saiPlatform = getSaiPlatform(portId);
  auto updateFn =
      [portId, saiPlatform, this, &sak](std::shared_ptr<SwitchState> in) {
        return portUpdateHelper(in, portId, saiPlatform, [&sak](auto& port) {
          if (port->getTxSak() && *port->getTxSak() == sak) {
            port->setTxSak(std::nullopt);
          }
        });
      };
  getPlatformInfo(portId)->applyUpdate(
      folly::sformat("Delete SAK tx for : {}", *sak.l2Port()), updateFn);
}

mka::MKASakHealthResponse SaiPhyManager::sakHealthCheck(
    const mka::MKASak sak) const {
  auto portId = getPortId(*sak.l2Port());
  mka::MKASakHealthResponse health;
  health.active() = false;
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
  health.active() = txActive && rxActive;
  health.lowestAcceptablePN() = 1;
  if (*health.active()) {
    auto macsecStats = getMacsecStats(*sak.l2Port());
    mka::MKASecureAssociationId saId;
    saId.sci() = *sak.sci();
    saId.assocNum() = *sak.assocNum();
    // Find the saStats for this sa
    for (auto& txSaStats : macsecStats.txSecureAssociationStats().value()) {
      if (txSaStats.saId()->sci().value() == sak.sci().value() &&
          txSaStats.saId()->assocNum().value() == sak.assocNum().value()) {
        health.lowestAcceptablePN() =
            txSaStats.saStats()->outEncryptedPkts().value();
        break;
      }
    }
  }
  return health;
}

bool SaiPhyManager::setupMacsecState(
    const std::vector<std::string>& portList,
    bool macsecDesired,
    bool dropUnencrypted) {
  for (const auto& port : portList) {
    auto portId = getPortId(port);
    // If macsecDesired is False and we are trying to remove Macsec from a list
    // of ports then from the portList, remove the ports which are not
    // programmed in HW for Macsec
    PlatformInfo* platInfo;
    try {
      platInfo = getPlatformInfo(portId);
    } catch (FbossError& e) {
      if (macsecDesired) {
        throw;
      } else {
        continue;
      }
    }
    auto saiPlatform = getSaiPlatform(portId);

    auto updatePortsFn =
        [saiPlatform, this, portId, macsecDesired, dropUnencrypted](
            std::shared_ptr<SwitchState> in) {
          auto portObj = in->getPorts()->getPortIf(portId);
          if (!portObj && !macsecDesired) {
            XLOG(DBG5) << "Port " << portId << " does not exists";
            return std::shared_ptr<SwitchState>(nullptr);
          }
          return portUpdateHelper(
              in,
              portId,
              saiPlatform,
              [macsecDesired, dropUnencrypted](auto& port) {
                port->setDropUnencrypted(dropUnencrypted);
                port->setMacsecDesired(macsecDesired);
              });
        };
    XLOG(DBG5) << "Port " << portId << " Applying update";
    getPlatformInfo(portId)->applyUpdate(
        folly::sformat(
            "Setup Macsec state for port: {:s} Macsec desired: {:s} Drop unencrypted: {:s}",
            port,
            (macsecDesired ? "True" : "False"),
            (dropUnencrypted ? "True" : "False")),
        updatePortsFn);
  }
  return true;
}

PortID SaiPhyManager::getPortId(std::string portName) const {
  auto portIdVal = folly::tryTo<int>(portName);
  if (portIdVal.hasValue()) {
    return PortID(portIdVal.value());
  }
  auto platPorts = getPlatformMapping()->getPlatformPorts();
  for (const auto& pair : platPorts) {
    if (folly::to<std::string>(*pair.second.mapping()->name()) == portName) {
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
    std::optional<TransceiverInfo> transceiverInfo,
    bool needResetDataPath) {
  bool isChanged{false};
  {
    const auto& wLockedCache = getWLockedCache(portId);

    // Get phy platform
    auto globalPhyID = getGlobalXphyIDbyPortIDLocked(wLockedCache);
    auto saiPlatform = getSaiPlatform(globalPhyID);
    auto saiSwitch = static_cast<SaiSwitch*>(saiPlatform->getHwSwitch());
    const auto& desiredPhyPortConfig =
        getDesiredPhyPortConfig(portId, portProfileId, transceiverInfo);

    // Is port create allowed
    if (!isPortCreateAllowed(globalPhyID, desiredPhyPortConfig)) {
      XLOG(INFO) << "PHY Port create not allowed for port " << portId;
      return;
    }

    // Before actually calling sai sdk to program the port again, we should
    // check whether the port has been programmed in HW with the same config.
    // We avoid reprogramming the port if the config matches with what we have
    // already programmed before, unless the needResetDataPath flag is set
    if (!needResetDataPath &&
        !(wLockedCache->systemLanes.empty() ||
          wLockedCache->lineLanes.empty())) {
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
    isChanged = setPortToPortCacheInfoLocked(
        wLockedCache, portId, portProfileId, desiredPhyPortConfig);
    // Only reset phy port stats when there're changes on the xphy ports
    if (isChanged &&
        (getExternalPhyLocked(wLockedCache)
             ->isSupported(phy::ExternalPhy::Feature::PORT_STATS) ||
         (getExternalPhyLocked(wLockedCache)
              ->isSupported(phy::ExternalPhy::Feature::PRBS_STATS)))) {
      setPortToExternalPhyPortStats(portId, createExternalPhyPortStats(portId));
    }
  }
  if (isChanged || needResetDataPath) {
    // If needed, tune the XPHY-NPU link again
    xphyPortStateToggle(portId, phy::Side::SYSTEM);
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

/*
 * getPhyInfo
 *
 * Get the macsec phy line side and system link information from
 * SaiPortManager SaiPortHandle attribute
 */
phy::PhyInfo SaiPhyManager::getPhyInfo(PortID swPort) {
  // Get phy platform
  auto globalPhyID = getGlobalXphyIDbyPortID(swPort);
  auto saiPlatform = getSaiPlatform(globalPhyID);
  auto saiSwitch = static_cast<SaiSwitch*>(saiPlatform->getHwSwitch());

  auto allPhyParams = saiSwitch->updateAllPhyInfo();
  if (allPhyParams.find(swPort) != allPhyParams.end()) {
    return allPhyParams[swPort];
  }
  return phy::PhyInfo{};
}

std::string SaiPhyManager::getSaiPortInfo(PortID swPort) {
  std::string output;

  XLOG(INFO) << "SaiPhyManager::getSaiPortInfo";
  auto globalPhyID = getGlobalXphyIDbyPortID(swPort);
  auto saiPlatform = getSaiPlatform(globalPhyID);
  auto saiSwitch = static_cast<SaiSwitch*>(saiPlatform->getHwSwitch());

  auto switchId = saiSwitch->getSaiSwitchId();

  // Get port handle and then get port attributes
  auto portHandle =
      saiSwitch->managerTable()->portManager().getPortHandle(swPort);

  if (portHandle == nullptr) {
    XLOG(INFO) << "Port Handle is null";
    return "";
  }

  output.append(folly::sformat(
      "SwPort {:d} \n Switch Id {:d}\n",
      static_cast<int>(swPort),
      static_cast<uint64_t>(switchId)));

  auto linePortAdapter = portHandle->port->adapterKey();
  auto sysPortAdapter = portHandle->sysPort->adapterKey();
  auto connectorAdapter = portHandle->connector->adapterKey();

  auto portInfoGet = [this](
                         bool lineSide,
                         SaiPortTraits::AdapterKey& portAdapter,
                         std::string& output) {
    output.append(folly::sformat(
        "  {:s} Port Obj = {}\n",
        (lineSide ? "Line" : "System"),
        portAdapter.t));

    auto portOperStatus = SaiApiTable::getInstance()->portApi().getAttribute(
        portAdapter, SaiPortTraits::Attributes::OperStatus{});
    output.append(folly::sformat(
        "    Link Status: {:d}\n", static_cast<int>(portOperStatus)));
    auto portSpeed = SaiApiTable::getInstance()->portApi().getAttribute(
        portAdapter, SaiPortTraits::Attributes::Speed{});
    output.append(folly::sformat("    Speed: {:d}\n", portSpeed));

    bool extendedFec{false};
    uint32_t fecMode, extFecMode;
#if SAI_API_VERSION >= SAI_VERSION(1, 10, 0)
    extendedFec = SaiApiTable::getInstance()->portApi().getAttribute(
        portAdapter, SaiPortTraits::Attributes::UseExtendedFec{});
    if (extendedFec) {
      extFecMode = SaiApiTable::getInstance()->portApi().getAttribute(
          portAdapter, SaiPortTraits::Attributes::ExtendedFecMode{});
    }
#endif
    if (!extendedFec) {
      fecMode = SaiApiTable::getInstance()->portApi().getAttribute(
          portAdapter, SaiPortTraits::Attributes::FecMode{});
    }
    output.append(folly::sformat(
        "    Extended Fec : {:s}\n", (extendedFec ? "True" : "False")));
    output.append(folly::sformat(
        "    Fec mode: {:d}\n", (extendedFec ? extFecMode : fecMode)));

    auto interfaceType = SaiApiTable::getInstance()->portApi().getAttribute(
        portAdapter, SaiPortTraits::Attributes::InterfaceType{});
    output.append(folly::sformat("    Interface Type: {:d}\n", interfaceType));
    auto serdesId = SaiApiTable::getInstance()->portApi().getAttribute(
        portAdapter, SaiPortTraits::Attributes::SerdesId{});
    output.append(folly::sformat("    Serdes Id: {:d}\n", serdesId));
  };

  portInfoGet(true, linePortAdapter, output);
  portInfoGet(false, sysPortAdapter, output);

  output.append(
      folly::sformat("  Port Connector Obj = {}\n", connectorAdapter.t));
  return output;
}

void SaiPhyManager::setSaiPortLoopbackState(
    PortID swPort,
    phy::PortComponent component,
    bool setLoopback) {
  XLOG(INFO) << "SaiPhyManager::setSaiPortLoopbackState";
  auto globalPhyID = getGlobalXphyIDbyPortID(swPort);
  auto saiPlatform = getSaiPlatform(globalPhyID);
  auto saiSwitch = static_cast<SaiSwitch*>(saiPlatform->getHwSwitch());

  auto switchId = saiSwitch->getSaiSwitchId();

  // Get port handle and then get port attributes
  auto portHandle =
      saiSwitch->managerTable()->portManager().getPortHandle(swPort);

  if (portHandle == nullptr) {
    XLOG(INFO) << "Port Handle is null";
    return;
  }

  auto portAdapterKey = (component == phy::PortComponent::GB_LINE)
      ? portHandle->port->adapterKey()
      : portHandle->sysPort->adapterKey();

#if SAI_API_VERSION >= SAI_VERSION(1, 10, 0)
  auto loopbackMode =
      setLoopback ? SAI_PORT_LOOPBACK_MODE_PHY : SAI_PORT_LOOPBACK_MODE_NONE;

  SaiApiTable::getInstance()->portApi().setAttribute(
      portAdapterKey,
      SaiPortTraits::Attributes::PortLoopbackMode{loopbackMode});
  XLOG(INFO) << folly::sformat(
      "Port {} Side {:s} Loopback state set to {:d}",
      static_cast<int>(swPort),
      ((component == phy::PortComponent::GB_LINE) ? "line" : "system"),
      static_cast<int>(loopbackMode));
#endif
}

void SaiPhyManager::setSaiPortAdminState(
    PortID swPort,
    phy::PortComponent component,
    bool setAdminUp) {
  XLOG(INFO) << "SaiPhyManager::setSaiPortAdminState";
  auto globalPhyID = getGlobalXphyIDbyPortID(swPort);
  auto saiPlatform = getSaiPlatform(globalPhyID);
  auto saiSwitch = static_cast<SaiSwitch*>(saiPlatform->getHwSwitch());

  auto switchId = saiSwitch->getSaiSwitchId();

  // Get port handle and then get port attributes
  auto portHandle =
      saiSwitch->managerTable()->portManager().getPortHandle(swPort);

  if (portHandle == nullptr) {
    XLOG(INFO) << "Port Handle is null";
    return;
  }

  auto portAdapterKey = (component == phy::PortComponent::GB_LINE)
      ? portHandle->port->adapterKey()
      : portHandle->sysPort->adapterKey();

  SaiApiTable::getInstance()->portApi().setAttribute(
      portAdapterKey, SaiPortTraits::Attributes::AdminState{setAdminUp});
  XLOG(INFO) << folly::sformat(
      "Port {} Side {:s} Admin state set to {:s}",
      static_cast<int>(swPort),
      ((component == phy::PortComponent::GB_LINE) ? "line" : "system"),
      (setAdminUp ? "Up" : "Down"));
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
  return hwPortStats.macsecStats() ? *hwPortStats.macsecStats() : MacsecStats{};
}

/*
 * getAllMacsecPortStats
 *
 * Get the macsec stats for all ports in the system. This function will loop
 * through all pim platforms and then all xphy in the pim platform and then
 * get macsec stats for all ports. This function returns the map of port name
 * to MacsecStats structure which contains port, flow and SA stats
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
            statsItr.second.macsecStats().has_value()
            ? statsItr.second.macsecStats().value()
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
      ? *macsecStats.ingressPortStats()
      : *macsecStats.egressPortStats();
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
      returnFlowStats.directionIngress() =
          singleFlowStat.flowStats()->directionIngress().value();
      returnFlowStats.ucastUncontrolledPkts() =
          returnFlowStats.ucastUncontrolledPkts().value() +
          singleFlowStat.flowStats()->ucastUncontrolledPkts().value();
      returnFlowStats.ucastControlledPkts() =
          returnFlowStats.ucastControlledPkts().value() +
          singleFlowStat.flowStats()->ucastControlledPkts().value();
      returnFlowStats.mcastUncontrolledPkts() =
          returnFlowStats.mcastUncontrolledPkts().value() +
          singleFlowStat.flowStats()->mcastUncontrolledPkts().value();
      returnFlowStats.mcastControlledPkts() =
          returnFlowStats.mcastControlledPkts().value() +
          singleFlowStat.flowStats()->mcastControlledPkts().value();
      returnFlowStats.bcastUncontrolledPkts() =
          returnFlowStats.bcastUncontrolledPkts().value() +
          singleFlowStat.flowStats()->bcastUncontrolledPkts().value();
      returnFlowStats.bcastControlledPkts() =
          returnFlowStats.bcastControlledPkts().value() +
          singleFlowStat.flowStats()->bcastControlledPkts().value();
      returnFlowStats.controlPkts() = returnFlowStats.controlPkts().value() +
          singleFlowStat.flowStats()->controlPkts().value();
      returnFlowStats.untaggedPkts() = returnFlowStats.untaggedPkts().value() +
          singleFlowStat.flowStats()->untaggedPkts().value();
      returnFlowStats.otherErrPkts() = returnFlowStats.otherErrPkts().value() +
          singleFlowStat.flowStats()->otherErrPkts().value();
      returnFlowStats.octetsUncontrolled() =
          returnFlowStats.octetsUncontrolled().value() +
          singleFlowStat.flowStats()->octetsUncontrolled().value();
      returnFlowStats.octetsControlled() =
          returnFlowStats.octetsControlled().value() +
          singleFlowStat.flowStats()->octetsControlled().value();
      returnFlowStats.outCommonOctets() =
          returnFlowStats.outCommonOctets().value() +
          singleFlowStat.flowStats()->outCommonOctets().value();
      returnFlowStats.outTooLongPkts() =
          returnFlowStats.outTooLongPkts().value() +
          singleFlowStat.flowStats()->outTooLongPkts().value();
      returnFlowStats.inTaggedControlledPkts() =
          returnFlowStats.inTaggedControlledPkts().value() +
          singleFlowStat.flowStats()->inTaggedControlledPkts().value();
      returnFlowStats.inNoTagPkts() = returnFlowStats.inNoTagPkts().value() +
          singleFlowStat.flowStats()->inNoTagPkts().value();
      returnFlowStats.inBadTagPkts() = returnFlowStats.inBadTagPkts().value() +
          singleFlowStat.flowStats()->inBadTagPkts().value();
      returnFlowStats.noSciPkts() = returnFlowStats.noSciPkts().value() +
          singleFlowStat.flowStats()->noSciPkts().value();
      returnFlowStats.unknownSciPkts() =
          returnFlowStats.unknownSciPkts().value() +
          singleFlowStat.flowStats()->unknownSciPkts().value();
      returnFlowStats.overrunPkts() = returnFlowStats.overrunPkts().value() +
          singleFlowStat.flowStats()->overrunPkts().value();
    }
    return returnFlowStats;
  };

  if (direction == mka::MacsecDirection::INGRESS) {
    return sumFlowStats(macsecStats.ingressFlowStats().value());
  } else {
    return sumFlowStats(macsecStats.egressFlowStats().value());
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
      returnSaStats.directionIngress() =
          singleSaStat.saStats()->directionIngress().value();
      returnSaStats.octetsEncrypted() =
          returnSaStats.octetsEncrypted().value() +
          singleSaStat.saStats()->octetsEncrypted().value();
      returnSaStats.octetsProtected() =
          returnSaStats.octetsProtected().value() +
          singleSaStat.saStats()->octetsProtected().value();
      returnSaStats.outEncryptedPkts() =
          returnSaStats.outEncryptedPkts().value() +
          singleSaStat.saStats()->outEncryptedPkts().value();
      returnSaStats.outProtectedPkts() =
          returnSaStats.outProtectedPkts().value() +
          singleSaStat.saStats()->outProtectedPkts().value();
      returnSaStats.inUncheckedPkts() =
          returnSaStats.inUncheckedPkts().value() +
          singleSaStat.saStats()->inUncheckedPkts().value();
      returnSaStats.inDelayedPkts() = returnSaStats.inDelayedPkts().value() +
          singleSaStat.saStats()->inDelayedPkts().value();
      returnSaStats.inLatePkts() = returnSaStats.inLatePkts().value() +
          singleSaStat.saStats()->inLatePkts().value();
      returnSaStats.inInvalidPkts() = returnSaStats.inInvalidPkts().value() +
          singleSaStat.saStats()->inInvalidPkts().value();
      returnSaStats.inNotValidPkts() = returnSaStats.inNotValidPkts().value() +
          singleSaStat.saStats()->inNotValidPkts().value();
      returnSaStats.inNoSaPkts() = returnSaStats.inNoSaPkts().value() +
          singleSaStat.saStats()->inNoSaPkts().value();
      returnSaStats.inUnusedSaPkts() = returnSaStats.inUnusedSaPkts().value() +
          singleSaStat.saStats()->inUnusedSaPkts().value();
      returnSaStats.inOkPkts() = returnSaStats.inOkPkts().value() +
          singleSaStat.saStats()->inOkPkts().value();
    }
    return returnSaStats;
  };

  if (direction == mka::MacsecDirection::INGRESS) {
    return sumSaStats(macsecStats.rxSecureAssociationStats().value());
  } else {
    return sumSaStats(macsecStats.txSecureAssociationStats().value());
  }
}

std::string SaiPhyManager::listHwObjects(
    std::vector<HwObjectType>& hwObjects,
    bool cached) {
  std::string resultStr = "";

  // Loop through all pim platforms
  for (auto& pimPlatformItr : saiPlatforms_) {
    auto& pimPlatform = pimPlatformItr.second;

    // Loop through all xphy in the pim
    for (auto& platformItr : pimPlatform) {
      GlobalXphyID xphyID = platformItr.first;

      // Get SaiSwitch using global xphy id
      auto saiSwitch = getSaiSwitch(xphyID);

      resultStr += folly::to<std::string>("Xphy Id: ", xphyID, "\n");
      resultStr += folly::to<std::string>(
          "Sai Switch Id: ", saiSwitch->getSaiSwitchId(), "\n");
      resultStr += saiSwitch->listObjects(hwObjects, cached);
    }
  }

  return resultStr;
}

bool SaiPhyManager::getSdkState(const std::string& fileName) {
  if (fileName.empty()) {
    XLOG(ERR) << "getSdkState get invalid filename";
    return false;
  }

  auto pimPlatformItr = saiPlatforms_.begin();
  if (pimPlatformItr == saiPlatforms_.end()) {
    return false;
  }
  auto& pimPlatform = pimPlatformItr->second;

  auto platformItr = pimPlatform.begin();
  if (platformItr == pimPlatform.end()) {
    return false;
  }

  GlobalXphyID xphyID = platformItr->first;
  // Get SaiSwitch using global xphy id and dump sdk debug state
  auto saiSwitch = getSaiSwitch(xphyID);
  saiSwitch->dumpDebugState(fileName);
  return true;
}

void SaiPhyManager::setPortPrbs(
    PortID portID,
    phy::Side side,
    const phy::PortPrbsState& prbs) {
  const auto& wLockedCache = getWLockedCache(portID);

  auto* xphy = getExternalPhyLocked(wLockedCache);
  if (!xphy->isSupported(phy::ExternalPhy::Feature::PRBS)) {
    throw FbossError("Port:", portID, " xphy can't support PRBS");
  }
  if (wLockedCache->systemLanes.empty() || wLockedCache->lineLanes.empty()) {
    throw FbossError(
        "Port:", portID, " is not programmed and can't find cached lanes");
  }

  // Configure PRBS using SAI
  auto* saiSwitch = getSaiSwitch(portID);
  auto portHandle =
      saiSwitch->managerTable()->portManager().getPortHandle(portID);
  const auto& sideLanes = side == phy::Side::SYSTEM ? wLockedCache->systemLanes
                                                    : wLockedCache->lineLanes;
  auto portAdapterKey = side == phy::Side::SYSTEM
      ? portHandle->sysPort->adapterKey()
      : portHandle->port->adapterKey();
  XLOG(INFO) << "Setting the PRBS on port " << portAdapterKey.t;

  if (prbs.enabled().value()) {
    SaiApiTable::getInstance()->portApi().setAttribute(
        portAdapterKey,
        SaiPortTraits::Attributes::PrbsPolynomial{prbs.polynominal().value()});
  }
  SaiApiTable::getInstance()->portApi().setAttribute(
      portAdapterKey,
      SaiPortTraits::Attributes::PrbsConfig{prbs.enabled().value() ? 1 : 0});

  // Enable stats collection
  const auto& wLockedStats = getWLockedStats(portID);
  if (*prbs.enabled()) {
    const auto& phyPortConfig = getHwPhyPortConfigLocked(wLockedCache, portID);
    wLockedStats->stats->setupPrbsCollection(
        side, sideLanes, phyPortConfig.getLaneSpeedInMb(side));
  } else {
    wLockedStats->stats->disablePrbsCollection(side);
  }
}

phy::PortPrbsState SaiPhyManager::getPortPrbs(PortID portID, phy::Side side) {
  const auto& rLockedCache = getRLockedCache(portID);

  auto* xphy = getExternalPhyLocked(rLockedCache);
  if (!xphy->isSupported(phy::ExternalPhy::Feature::PRBS)) {
    throw FbossError("Port:", portID, " xphy can't support PRBS");
  }
  if (rLockedCache->systemLanes.empty() || rLockedCache->lineLanes.empty()) {
    throw FbossError(
        "Port:", portID, " is not programmed and can't find cached lanes");
  }

  // Get Port PRBS state from SAI
  phy::PortPrbsState prbsState;
  auto* saiSwitch = getSaiSwitch(portID);
  auto portHandle =
      saiSwitch->managerTable()->portManager().getPortHandle(portID);
  auto portAdapterKey = side == phy::Side::LINE
      ? portHandle->port->adapterKey()
      : portHandle->sysPort->adapterKey();
  XLOG(INFO) << "Getting the PRBS state from port " << portAdapterKey.t;

  prbsState.polynominal() = SaiApiTable::getInstance()->portApi().getAttribute(
      portAdapterKey, SaiPortTraits::Attributes::PrbsPolynomial{});
  prbsState.enabled() =
      SaiApiTable::getInstance()->portApi().getAttribute(
          portAdapterKey, SaiPortTraits::Attributes::PrbsConfig{}) != 0;
  return prbsState;
}

std::vector<phy::PrbsLaneStats> SaiPhyManager::getPortPrbsStats(
    PortID portID,
    phy::Side side) {
  const auto& rLockedCache = getRLockedCache(portID);
  auto* xphy = getExternalPhyLocked(rLockedCache);
  if (!xphy->isSupported(phy::ExternalPhy::Feature::PRBS_STATS)) {
    throw FbossError("Port:", portID, " xphy can't support PRBS_STATS");
  }
  if (rLockedCache->lineLanes.empty() || rLockedCache->systemLanes.empty()) {
    throw FbossError(
        "Port:", portID, " xphy syslem and line lanes not configured yet");
  }

  // Get PRBS info from SAI
  auto* saiSwitch = getSaiSwitch(portID);
  auto portHandle =
      saiSwitch->managerTable()->portManager().getPortHandle(portID);
  auto portAdapterKey = side == phy::Side::SYSTEM
      ? portHandle->sysPort->adapterKey()
      : portHandle->port->adapterKey();
  XLOG(INFO) << "Getting the PRBS state from port " << portAdapterKey.t;

  std::vector<phy::PrbsLaneStats> lanePrbs;
  phy::PrbsLaneStats oneLanePrbs;
  oneLanePrbs.laneId() = 0;

  bool prbsEnabled =
      SaiApiTable::getInstance()->portApi().getAttribute(
          portAdapterKey, SaiPortTraits::Attributes::PrbsConfig{}) != 0;

  if (prbsEnabled) {
#if SAI_API_VERSION >= SAI_VERSION(1, 8, 1)
    auto prbsState = SaiApiTable::getInstance()->portApi().getAttribute(
        portAdapterKey, SaiPortTraits::Attributes::PrbsRxState{});
    oneLanePrbs.locked() = prbsState.rx_status > 0;
    oneLanePrbs.ber() = prbsState.error_count;
#endif
  }

  lanePrbs.push_back(oneLanePrbs);
  return lanePrbs;
}

/*
 * xphyPortStateToggle
 *
 * This function toggles the XPHY given side port. This is helpful to make the
 * NPU Rx to tune with XPHY host side Tx. The NPU IPHY port gets programmed
 * first so after the XPHY port gets created then we need to turn off and on the
 * XPHY host Tx to let the NPU IPHY Rx lock to the correct signal
 */
void SaiPhyManager::xphyPortStateToggle(PortID swPort, phy::Side side) {
  XLOG(INFO) << "SaiPhyManager::xphyPortStateToggle";
  auto globalPhyID = getGlobalXphyIDbyPortID(swPort);
  auto saiPlatform = getSaiPlatform(globalPhyID);
  if (!saiPlatform->getAsic()->isSupported(
          HwAsic::Feature::XPHY_PORT_STATE_TOGGLE)) {
    XLOG(ERR) << "xphyPortStateToggle: Feature not supported";
    return;
  }

  auto saiSwitch = static_cast<SaiSwitch*>(saiPlatform->getHwSwitch());
  auto switchId = saiSwitch->getSaiSwitchId();

  // Get port handle and then get port adapter key (SAI object id)
  auto portHandle =
      saiSwitch->managerTable()->portManager().getPortHandle(swPort);

  if (portHandle == nullptr) {
    XLOG(INFO) << "Port Handle is null";
    return;
  }

  auto portAdapterKey = (side == phy::Side::SYSTEM)
      ? portHandle->sysPort->adapterKey()
      : portHandle->port->adapterKey();

  // Flap XPHY side port to let the peer (NPU IPHY Rx or other end) lock to
  // the correct signal again
  SaiApiTable::getInstance()->portApi().setAttribute(
      portAdapterKey, SaiPortTraits::Attributes::AdminState{false});
  /* sleep override */
  usleep(100000);
  SaiApiTable::getInstance()->portApi().setAttribute(
      portAdapterKey, SaiPortTraits::Attributes::AdminState{true});
  /* sleep override */
  usleep(100000);
  XLOG(INFO) << "Sai port " << swPort
             << (side == phy::Side::SYSTEM ? " Host" : " Line")
             << " Xphy port toggle done";
}

/*
 * gracefulExit
 *
 * This function calls SaiSwitch::gracefulExit() which will set the Sai Graceful
 * Restart attribute. That function will store the SaiSwitch state in a file and
 * also let the SAI SDK store its state in a file
 */
void SaiPhyManager::gracefulExit() {
  // Loop through all pim platforms
  for (auto& pimPlatformItr : saiPlatforms_) {
    auto& pimPlatform = pimPlatformItr.second;

    // Loop through all xphy in the pim
    for (auto& platformItr : pimPlatform) {
      GlobalXphyID xphyID = platformItr.first;
      if (!getSaiPlatform(xphyID)->getAsic()->isSupported(
              HwAsic::Feature::XPHY_SAI_WARMBOOT)) {
        XLOG(DBG3) << "gracefulExit: Warmboot not supported for this platform";
        return;
      }

      // Get SaiSwitch and SwitchState using global xphy id
      auto saiSwitch = getSaiSwitch(xphyID);
      auto switchState = getPlatformInfo(xphyID)->getState();

      // Get the current SwitchState and ThriftState which will be used to call
      //  SaiSwitch::gracefulExit function
      folly::dynamic follySwitchState = folly::dynamic::object;
      follySwitchState[kSwSwitch] = switchState->toFollyDynamic();
      state::WarmbootState thriftSwitchState;
      *thriftSwitchState.swSwitchState() = switchState->toThrift();
      saiSwitch->gracefulExit(follySwitchState, thriftSwitchState);
    }
  }
}

} // namespace facebook::fboss
