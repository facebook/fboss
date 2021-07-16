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

#include "fboss/agent/hw/sai/api/PortApi.h"
#include "fboss/agent/hw/sai/api/SaiApiTable.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiPortManager.h"
#include "fboss/agent/hw/sai/switch/SaiSwitch.h"
#include "fboss/lib/phy/PhyManager.h"
#include "fboss/lib/phy/SaiPhyManager.h"
#include "fboss/qsfp_service/test/hw_test/HwQsfpEnsemble.h"

#include <folly/logging/xlog.h>
#include <gtest/gtest.h>
#include <thrift/lib/cpp/util/EnumUtils.h>

namespace facebook::fboss::utility {

void verifyPhyPortConfig(
    PortID portID,
    PhyManager* phyManager,
    const phy::PhyPortConfig& expectedConfig) {
  // Now fetch the config actually programmed to the xphy
  const auto& actualPortConfig = phyManager->getHwPhyPortConfig(portID);

  // Check speed
  EXPECT_EQ(expectedConfig.profile.speed, actualPortConfig.profile.speed);
  // Check ProfileSideConfig. Due to we couldn't fetch all the config yet.
  // Just check the attribures we can get
  auto checkProfileSideConfig = [&](const phy::ProfileSideConfig& expected,
                                    const phy::ProfileSideConfig& actual) {
    EXPECT_EQ(expected.get_numLanes(), actual.get_numLanes());
    EXPECT_EQ(expected.get_fec(), actual.get_fec());
    EXPECT_EQ(expected.get_modulation(), actual.get_modulation());
    if (auto interfaceType = actual.interfaceType_ref()) {
      EXPECT_EQ(expected.interfaceType_ref(), interfaceType);
    }
    // TODO(joseph5wu) Need to deprecate interfaceMode
    if (auto interfaceMode = actual.interfaceMode_ref()) {
      EXPECT_EQ(expected.interfaceMode_ref(), interfaceMode);
    }
  };
  checkProfileSideConfig(
      expectedConfig.profile.system, actualPortConfig.profile.system);
  checkProfileSideConfig(
      expectedConfig.profile.line, actualPortConfig.profile.line);

  // Currently we don't return tx_settings for Elbert system yet.
  // Only check phy::ExternalPhyConfig when the actual config exists
  const auto& actualSysLanesConf = actualPortConfig.config.system.lanes;
  const auto& acutualLineLanesConf = actualPortConfig.config.line.lanes;
  if (!actualSysLanesConf.empty() && !acutualLineLanesConf.empty()) {
    EXPECT_EQ(expectedConfig.config, actualPortConfig.config);
  } else {
    XLOG(DBG2) << "Actuall hardware config doesn't have ExternalPhyConfig";
  }
}

void verifyPhyPortConnector(PortID portID, HwQsfpEnsemble* qsfpEnsemble) {
  if (qsfpEnsemble->getWedgeManager()->getPlatformMode() !=
      PlatformMode::ELBERT) {
    return;
  }

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
PortStatus getPortStatus(PortID portId, const HwQsfpEnsemble* ensemble) {
  auto transceiverId = getTranscieverIdx(portId, ensemble);
  EXPECT_TRUE(transceiverId);
  auto config = *ensemble->getWedgeManager()->getAgentConfig()->thrift.sw_ref();
  std::optional<cfg::Port> portCfg;
  for (auto& port : *config.ports_ref()) {
    if (*port.logicalID_ref() == static_cast<uint16_t>(portId)) {
      portCfg = port;
      break;
    }
  }
  CHECK(portCfg);
  PortStatus status;
  status.enabled_ref() = *portCfg->state_ref() == cfg::PortState::ENABLED;
  TransceiverIdxThrift idx;
  idx.transceiverId_ref() = *transceiverId;
  status.transceiverIdx_ref() = idx;
  // Mark port down to force transceiver programming
  status.up_ref() = false;
  status.speedMbps_ref() = static_cast<int64_t>(*portCfg->speed_ref());
  return status;
}

IphyAndXphyPorts findAvailablePorts(
    HwQsfpEnsemble* hwQsfpEnsemble,
    std::optional<cfg::PortProfileID> profile) {
  std::set<PortID> xPhyPorts;
  const auto& platformPorts =
      hwQsfpEnsemble->getPlatformMapping()->getPlatformPorts();
  const auto& chips = hwQsfpEnsemble->getPlatformMapping()->getChips();
  IphyAndXphyPorts ports;
  auto agentConfig = hwQsfpEnsemble->getWedgeManager()->getAgentConfig();
  auto& swConfig = *agentConfig->thrift.sw_ref();
  for (auto& port : *swConfig.ports_ref()) {
    if (*port.state_ref() != cfg::PortState::ENABLED) {
      continue;
    }
    if (profile && *port.profileID_ref() != *profile) {
      continue;
    }

    auto portIDAndProfile =
        std::make_pair(*port.logicalID_ref(), *port.profileID_ref());
    const auto& xphy = utility::getDataPlanePhyChips(
        platformPorts.find(*port.logicalID_ref())->second,
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

std::vector<PortID> getCabledPorts(const AgentConfig& config) {
  std::vector<PortID> cabledPorts;
  auto& swConfig = *config.thrift.sw_ref();
  for (auto& port : *swConfig.ports_ref()) {
    if (!(*port.expectedLLDPValues_ref()).empty()) {
      cabledPorts.push_back(PortID(*port.logicalID_ref()));
    }
  }
  return cabledPorts;
}

std::vector<TransceiverID> getCabledPortTranceivers(
    const AgentConfig& config,
    const HwQsfpEnsemble* ensemble) {
  std::vector<TransceiverID> transceivers;
  for (auto port : getCabledPorts(config)) {
    transceivers.push_back(*getTranscieverIdx(port, ensemble));
  }
  return transceivers;
}

bool match(std::vector<TransceiverID> l, std::vector<TransceiverID> r) {
  std::sort(r.begin(), r.end());
  std::sort(l.begin(), l.end());
  return l == r;
}
} // namespace facebook::fboss::utility
