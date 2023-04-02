/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/qsfp_service/test/hw_test/HwPortUtils.h"

#include "fboss/agent/AgentConfig.h"

#ifndef IS_OSS
#include "fboss/agent/hw/sai/api/PortApi.h"
#include "fboss/agent/hw/sai/api/SaiApiTable.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiPortManager.h"
#include "fboss/agent/hw/sai/switch/SaiSwitch.h"
#include "fboss/lib/phy/SaiPhyManager.h"
#endif
#include "fboss/lib/phy/PhyManager.h"
#include "fboss/qsfp_service/test/hw_test/HwQsfpEnsemble.h"

#include <folly/logging/xlog.h>
#include <gtest/gtest.h>
#include <thrift/lib/cpp/util/EnumUtils.h>

namespace facebook::fboss::utility {

void fillDefaultTxSettings(PhyManager* phyManager, phy::PhyPortConfig& config) {
  auto fillSide = [&phyManager](auto&& phySideConfig) {
    for (auto& [_, laneConfig] : phySideConfig.lanes) {
      if (!laneConfig.tx) {
        laneConfig.tx = phyManager->getDefaultTxSettings();
      }
    }
  };
  fillSide(config.config.system);
  fillSide(config.config.line);
}

void verifyPhyPortConfig(
    PortID portID,
    PhyManager* phyManager,
    const phy::PhyPortConfig& expectedConfig,
    PlatformType platformMode) {
  phy::PhyPortConfig filledConfig = expectedConfig;
  // fill default TX settings in the expectedConfig
  fillDefaultTxSettings(phyManager, filledConfig);

  // Now fetch the config actually programmed to the xphy
  const auto& actualPortConfig = phyManager->getHwPhyPortConfig(portID);

  // Check speed
  EXPECT_EQ(filledConfig.profile.speed, actualPortConfig.profile.speed);
  // Check ProfileSideConfig. Due to we couldn't fetch all the config yet.
  // Just check the attribures we can get
  auto checkProfileSideConfig = [&](const phy::ProfileSideConfig& expected,
                                    const phy::ProfileSideConfig& actual) {
    EXPECT_EQ(expected.get_numLanes(), actual.get_numLanes());
    EXPECT_EQ(expected.get_fec(), actual.get_fec());
    EXPECT_EQ(expected.get_modulation(), actual.get_modulation());
    if (auto interfaceType = actual.interfaceType()) {
      EXPECT_EQ(expected.interfaceType(), interfaceType);
    }
    // TODO(joseph5wu) Need to deprecate interfaceMode
    if (auto interfaceMode = actual.interfaceMode()) {
      EXPECT_EQ(expected.interfaceMode(), interfaceMode);
    }
  };
  checkProfileSideConfig(
      filledConfig.profile.system, actualPortConfig.profile.system);
  checkProfileSideConfig(
      filledConfig.profile.line, actualPortConfig.profile.line);

  // Currently we don't return tx_settings for Elbert system yet.
  // Only check phy::ExternalPhyConfig when the actual config exists
  const auto& actualSysLanesConf = actualPortConfig.config.system.lanes;
  const auto& acutualLineLanesConf = actualPortConfig.config.line.lanes;
  // FIXME: remove Sandia check when we start reading tx settings on Sandia XPHY
  if (!actualSysLanesConf.empty() && !acutualLineLanesConf.empty() &&
      platformMode != PlatformType::PLATFORM_SANDIA) {
    EXPECT_EQ(filledConfig.config, actualPortConfig.config)
        << " Mismatch between expected and actual port config.\nExpected "
        << filledConfig.config.toDynamic()
        << "\n Actual: " << actualPortConfig.config.toDynamic();
  } else {
    XLOG(DBG2) << "Actuall hardware config doesn't have ExternalPhyConfig";
  }
}

void verifyPhyPortConnector(PortID portID, HwQsfpEnsemble* qsfpEnsemble) {
  // FIXME: remove Sandia check when Sandia XPHY supports getattr for
  // PortConnector object in SAI
  if (!qsfpEnsemble->isSaiPlatform() ||
      qsfpEnsemble->getWedgeManager()->getPlatformType() ==
          PlatformType::PLATFORM_SANDIA) {
    return;
  }
  // FIXME: [oss-fix] Remove this when linking to a SAI library is supported in
  // OSS build
#ifndef IS_OSS
  auto saiPhyManager =
      static_cast<SaiPhyManager*>(qsfpEnsemble->getPhyManager());
  // The goal here is to check whether:
  // 1) the connector is using the correct system port and line port.
  // 2) both system and line port admin state is enabled
  // Expected config from PlatformMapping
  auto* saiPortHandle = saiPhyManager->getSaiSwitch(portID)
                            ->managerTable()
                            ->portManager()
                            .getPortHandle(portID);
  auto connector = saiPortHandle->connector;
  auto& portApi = SaiApiTable::getInstance()->portApi();
  auto linePortID = portApi.getAttribute(
      connector->adapterKey(),
      SaiPortConnectorTraits::Attributes::LineSidePortId{});
  EXPECT_EQ(linePortID, saiPortHandle->port->adapterKey());

  auto systemPortID = portApi.getAttribute(
      connector->adapterKey(),
      SaiPortConnectorTraits::Attributes::SystemSidePortId{});
  EXPECT_EQ(systemPortID, saiPortHandle->sysPort->adapterKey());

  auto adminState = portApi.getAttribute(
      saiPortHandle->port->adapterKey(),
      SaiPortTraits::Attributes::AdminState{});
  EXPECT_TRUE(adminState);

  adminState = portApi.getAttribute(
      saiPortHandle->sysPort->adapterKey(),
      SaiPortTraits::Attributes::AdminState{});
  EXPECT_TRUE(adminState);
#endif
}

std::optional<TransceiverID> getTranscieverIdx(
    PortID portId,
    const HwQsfpEnsemble* ensemble) {
  const auto& platformPorts =
      ensemble->getPlatformMapping()->getPlatformPorts();
  const auto& chips = ensemble->getPlatformMapping()->getChips();
  return utility::getTransceiverId(
      platformPorts.find(static_cast<int32_t>(portId))->second, chips);
}

std::vector<TransceiverID> getTransceiverIds(
    const std::vector<PortID>& ports,
    const HwQsfpEnsemble* ensemble,
    bool ignoreNotFound) {
  std::vector<TransceiverID> transceivers;
  for (auto port : ports) {
    auto tid = getTranscieverIdx(port, ensemble);
    CHECK(tid.has_value() || ignoreNotFound);
    if (tid) {
      transceivers.push_back(*tid);
    }
  }
  return transceivers;
}

PortStatus getPortStatus(PortID portId, const HwQsfpEnsemble* ensemble) {
  auto transceiverId = getTranscieverIdx(portId, ensemble);
  EXPECT_TRUE(transceiverId);
  auto config = *ensemble->getWedgeManager()->getAgentConfig()->thrift.sw();
  std::optional<cfg::Port> portCfg;
  for (auto& port : *config.ports()) {
    if (*port.logicalID() == static_cast<uint16_t>(portId)) {
      portCfg = port;
      break;
    }
  }
  CHECK(portCfg);
  PortStatus status;
  status.enabled() = *portCfg->state() == cfg::PortState::ENABLED;
  TransceiverIdxThrift idx;
  idx.transceiverId() = *transceiverId;
  status.transceiverIdx() = idx;
  // Mark port down to force transceiver programming
  status.up() = false;
  status.speedMbps() = static_cast<int64_t>(*portCfg->speed());
  return status;
}

IphyAndXphyPorts findAvailablePorts(
    HwQsfpEnsemble* hwQsfpEnsemble,
    std::optional<cfg::PortProfileID> profile,
    bool onlyCabled) {
  std::set<PortID> xPhyPorts;
  const auto& platformPorts =
      hwQsfpEnsemble->getPlatformMapping()->getPlatformPorts();
  const auto& chips = hwQsfpEnsemble->getPlatformMapping()->getChips();
  IphyAndXphyPorts ports;
  auto agentConfig = hwQsfpEnsemble->getWedgeManager()->getAgentConfig();
  auto cabledPorts = getCabledPorts(*agentConfig);
  auto& swConfig = *agentConfig->thrift.sw();
  auto isPortCabled = [&cabledPorts](PortID port) -> bool {
    return cabledPorts.find(port) != cabledPorts.end();
  };
  for (auto& port : *swConfig.ports()) {
    if (onlyCabled && !isPortCabled(PortID(*port.logicalID()))) {
      continue;
    }
    if (*port.state() != cfg::PortState::ENABLED) {
      continue;
    }
    if (profile && *port.profileID() != *profile) {
      continue;
    }

    auto portIDAndProfile =
        std::make_pair(*port.logicalID(), *port.profileID());
    const auto& xphy = utility::getDataPlanePhyChips(
        platformPorts.find(*port.logicalID())->second,
        chips,
        phy::DataPlanePhyChipType::XPHY);
    if (!xphy.empty()) {
      ports.xphyPorts.emplace_back(portIDAndProfile);
    }
    // All ports have iphys
    ports.iphyPorts.emplace_back(portIDAndProfile);
  }
  return ports;
}

std::set<PortID> getCabledPorts(const AgentConfig& config) {
  std::set<PortID> cabledPorts;
  auto& swConfig = *config.thrift.sw();
  for (auto& port : *swConfig.ports()) {
    if (!(*port.expectedLLDPValues()).empty()) {
      cabledPorts.insert(PortID(*port.logicalID()));
    }
  }
  return cabledPorts;
}

std::vector<TransceiverID> getCabledPortTranceivers(
    const AgentConfig& config,
    const HwQsfpEnsemble* ensemble) {
  std::unordered_set<TransceiverID> transceivers;
  // There could be multiple ports in a single transceiver. Therefore use a set
  // to get unique cabled transceivers
  for (auto port : getCabledPorts(config)) {
    transceivers.insert(*getTranscieverIdx(port, ensemble));
  }
  return std::vector<TransceiverID>(transceivers.begin(), transceivers.end());
}

bool match(std::vector<TransceiverID> l, std::vector<TransceiverID> r) {
  std::sort(r.begin(), r.end());
  std::sort(l.begin(), l.end());
  return l == r;
}

bool containsSubset(
    std::vector<TransceiverID> superset,
    std::vector<TransceiverID> subset) {
  std::sort(superset.begin(), superset.end());
  std::sort(subset.begin(), subset.end());
  return std::includes(
      superset.begin(), superset.end(), subset.begin(), subset.end());
}

std::vector<int32_t> legacyTransceiverIds(
    const std::vector<TransceiverID>& ids) {
  std::vector<int32_t> legacyIds;
  std::for_each(ids.begin(), ids.end(), [&legacyIds](auto id) {
    legacyIds.push_back(id);
  });
  return legacyIds;
}

void verifyXphyPort(
    PortID portID,
    cfg::PortProfileID profileID,
    std::optional<TransceiverInfo> tcvrOpt,
    HwQsfpEnsemble* ensemble) {
  auto* phyManager = ensemble->getPhyManager();
  const auto& expectedPhyPortConfig =
      phyManager->getDesiredPhyPortConfig(portID, profileID, tcvrOpt);

  // Make sure we cached the correct port profile and speed
  EXPECT_EQ(*phyManager->getProgrammedProfile(portID), profileID);
  EXPECT_EQ(
      *phyManager->getProgrammedSpeed(portID),
      expectedPhyPortConfig.profile.speed);

  utility::verifyPhyPortConfig(
      portID,
      ensemble->getPhyManager(),
      expectedPhyPortConfig,
      ensemble->getWedgeManager()->getPlatformType());

  utility::verifyPhyPortConnector(portID, ensemble);
}
} // namespace facebook::fboss::utility
