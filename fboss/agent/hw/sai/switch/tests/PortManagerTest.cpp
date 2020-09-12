/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/HwPortFb303Stats.h"
#include "fboss/agent/hw/StatsConstants.h"
#include "fboss/agent/hw/sai/api/SaiApiTable.h"
#include "fboss/agent/hw/sai/fake/FakeSai.h"
#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiPortManager.h"
#include "fboss/agent/hw/sai/switch/tests/ManagerTestBase.h"
#include "fboss/agent/platforms/sai/SaiPlatform.h"
#include "fboss/agent/platforms/sai/SaiPlatformPort.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/types.h"

#include <fb303/ServiceData.h>

#include <string>

#include <gtest/gtest.h>

using namespace facebook::fboss;

class PortManagerTest : public ManagerTestBase {
 public:
  void SetUp() override {
    setupStage = SetupStage::BLANK;
    ManagerTestBase::SetUp();
    p0 = testInterfaces[0].remoteHosts[0].port;
    p1 = testInterfaces[1].remoteHosts[0].port;
  }
  // TODO: make it properly handle different lanes/speeds for different
  // port ids...
  void checkPort(
      const PortID& swId,
      PortSaiId saiId,
      bool enabled,
      sai_uint32_t mtu = 9412) {
    // Check SaiPortApi perspective
    auto& portApi = saiApiTable->portApi();
    SaiPortTraits::Attributes::AdminState adminStateAttribute;
    SaiPortTraits::Attributes::HwLaneList hwLaneListAttribute;
    SaiPortTraits::Attributes::Speed speedAttribute;
    SaiPortTraits::Attributes::FecMode fecMode;
    SaiPortTraits::Attributes::InternalLoopbackMode ilbMode;
    SaiPortTraits::Attributes::Mtu mtuAttribute;
    auto gotAdminState = portApi.getAttribute(saiId, adminStateAttribute);
    EXPECT_EQ(enabled, gotAdminState);
    auto gotLanes = portApi.getAttribute(saiId, hwLaneListAttribute);
    EXPECT_EQ(1, gotLanes.size());
    EXPECT_EQ(swId, gotLanes[0]);
    auto gotSpeed = portApi.getAttribute(saiId, speedAttribute);
    EXPECT_EQ(25000, gotSpeed);
    auto gotFecMode = portApi.getAttribute(saiId, fecMode);
    EXPECT_EQ(static_cast<int32_t>(SAI_PORT_FEC_MODE_NONE), gotFecMode);
    auto gotIlbMode = portApi.getAttribute(saiId, ilbMode);
    EXPECT_EQ(
        static_cast<int32_t>(SAI_PORT_INTERNAL_LOOPBACK_MODE_NONE), gotIlbMode);
    auto gotMtu = portApi.getAttribute(saiId, mtuAttribute);
    EXPECT_EQ(mtu, gotMtu);
  }

  /**
   * DO NOT use this routine for adding ports.
   * This is only used to verify port consolidation logic in sai port
   * manager. It makes certain assumptions about the lane numbers and
   * also adds the port under the hood bypassing the port manager.
   */
  PortSaiId addPort(const PortID& swId, cfg::PortSpeed portSpeed) {
    auto& portApi = saiApiTable->portApi();
    std::vector<uint32_t> ls;
    if (portSpeed == cfg::PortSpeed::TWENTYFIVEG) {
      ls.push_back(swId);
    } else {
      ls.push_back(swId);
      ls.push_back(swId + 1);
      ls.push_back(swId + 2);
      ls.push_back(swId + 3);
    }
    SaiPortTraits::Attributes::AdminState adminState{true};
    SaiPortTraits::Attributes::HwLaneList lanes(ls);
    SaiPortTraits::Attributes::Speed speed{static_cast<int>(portSpeed)};
    SaiPortTraits::CreateAttributes a{lanes,
                                      speed,
                                      adminState,
                                      std::nullopt,
                                      std::nullopt,
                                      std::nullopt,
                                      std::nullopt,
                                      std::nullopt,
                                      std::nullopt,
                                      std::nullopt,
                                      std::nullopt,
                                      std::nullopt,
                                      std::nullopt,
                                      std::nullopt};
    return portApi.create<SaiPortTraits>(a, 0);
  }

  void checkSubsumedPorts(
      TestPort port,
      cfg::PortSpeed speed,
      std::vector<PortID> expectedPorts) {
    std::shared_ptr<Port> swPort = makePort(port);
    swPort->setSpeed(speed);
    saiManagerTable->portManager().addPort(swPort);
    SaiPlatformPort* platformPort = saiPlatform->getPort(swPort->getID());
    EXPECT_TRUE(platformPort);
    auto subsumedPorts = platformPort->getSubsumedPorts(speed);
    EXPECT_EQ(subsumedPorts, expectedPorts);
    saiManagerTable->portManager().removePort(swPort);
  }

  TestPort p0;
  TestPort p1;
};

enum class ExpectExport { NO_EXPORT, EXPORT };

TEST_F(PortManagerTest, addPort) {
  std::shared_ptr<Port> swPort = makePort(p0);
  auto saiId = saiManagerTable->portManager().addPort(swPort);
  checkPort(PortID(0), saiId, true);
}

TEST_F(PortManagerTest, addTwoPorts) {
  std::shared_ptr<Port> swPort = makePort(p0);
  saiManagerTable->portManager().addPort(swPort);
  std::shared_ptr<Port> port2 = makePort(p1);
  auto saiId2 = saiManagerTable->portManager().addPort(port2);
  checkPort(PortID(10), saiId2, true);
}

TEST_F(PortManagerTest, iterator) {
  std::set<PortSaiId> addedPorts;
  auto& portMgr = saiManagerTable->portManager();
  std::shared_ptr<Port> swPort = makePort(p0);
  addedPorts.insert(portMgr.addPort(swPort));
  std::shared_ptr<Port> port2 = makePort(p1);
  addedPorts.insert(portMgr.addPort(port2));
  for (const auto& portIdAnHandle : portMgr) {
    auto& handle = portIdAnHandle.second;
    EXPECT_TRUE(
        addedPorts.find(handle->port->adapterKey()) != addedPorts.end());
    addedPorts.erase(handle->port->adapterKey());
  }
  EXPECT_EQ(0, addedPorts.size());
}

TEST_F(PortManagerTest, addDupIdPorts) {
  std::shared_ptr<Port> swPort = makePort(p0);
  saiManagerTable->portManager().addPort(swPort);
  EXPECT_THROW(saiManagerTable->portManager().addPort(swPort), FbossError);
}

TEST_F(PortManagerTest, getBySwId) {
  std::shared_ptr<Port> swPort = makePort(p1);
  saiManagerTable->portManager().addPort(swPort);
  SaiPortHandle* port =
      saiManagerTable->portManager().getPortHandle(PortID(10));
  EXPECT_TRUE(port);
  EXPECT_EQ(GET_OPT_ATTR(Port, AdminState, port->port->attributes()), true);
  EXPECT_EQ(GET_ATTR(Port, Speed, port->port->attributes()), 25000);
  auto hwLaneList = GET_ATTR(Port, HwLaneList, port->port->attributes());
  EXPECT_EQ(hwLaneList.size(), 1);
  EXPECT_EQ(hwLaneList[0], 10);
}

TEST_F(PortManagerTest, getNonExistent) {
  std::shared_ptr<Port> swPort = makePort(p0);
  saiManagerTable->portManager().addPort(swPort);
  SaiPortHandle* port =
      saiManagerTable->portManager().getPortHandle(PortID(10));
  EXPECT_FALSE(port);
}

TEST_F(PortManagerTest, removePort) {
  std::shared_ptr<Port> swPort = makePort(p0);
  saiManagerTable->portManager().addPort(swPort);
  SaiPortHandle* port = saiManagerTable->portManager().getPortHandle(PortID(0));
  EXPECT_TRUE(port);
  saiManagerTable->portManager().removePort(swPort);
  port = saiManagerTable->portManager().getPortHandle(PortID(0));
  EXPECT_FALSE(port);
}

TEST_F(PortManagerTest, removeNonExistentPort) {
  std::shared_ptr<Port> swPort = makePort(p0);
  saiManagerTable->portManager().addPort(swPort);
  SaiPortHandle* port = saiManagerTable->portManager().getPortHandle(PortID(0));
  EXPECT_TRUE(port);
  TestPort p10;
  p10.id = 10;
  std::shared_ptr<Port> swPort10 = makePort(p10);
  EXPECT_THROW(saiManagerTable->portManager().removePort(swPort10), FbossError);
}

TEST_F(PortManagerTest, changePortAdminState) {
  std::shared_ptr<Port> swPort = makePort(p0);
  auto saiId = saiManagerTable->portManager().addPort(swPort);
  swPort->setAdminState(cfg::PortState::DISABLED);
  saiManagerTable->portManager().changePort(swPort, swPort);
  checkPort(swPort->getID(), saiId, false);
}

TEST_F(PortManagerTest, changePortMtu) {
  std::shared_ptr<Port> swPort = makePort(p0);
  auto saiId = saiManagerTable->portManager().addPort(swPort);
  swPort->setMaxFrameSize(9000);
  saiManagerTable->portManager().changePort(swPort, swPort);
  checkPort(swPort->getID(), saiId, true, 9000);
}

TEST_F(PortManagerTest, changePortNoChange) {
  std::shared_ptr<Port> swPort = makePort(p0);
  auto saiId = saiManagerTable->portManager().addPort(swPort);
  swPort->setSpeed(cfg::PortSpeed::TWENTYFIVEG);
  swPort->setAdminState(cfg::PortState::ENABLED);
  saiManagerTable->portManager().changePort(swPort, swPort);
  checkPort(swPort->getID(), saiId, true);
}

TEST_F(PortManagerTest, changeNonExistentPort) {
  std::shared_ptr<Port> swPort = makePort(p0);
  std::shared_ptr<Port> swPort2 = makePort(p1);
  saiManagerTable->portManager().addPort(swPort);
  swPort2->setSpeed(cfg::PortSpeed::TWENTYFIVEG);
  EXPECT_THROW(
      saiManagerTable->portManager().changePort(swPort2, swPort2), FbossError);
}

TEST_F(PortManagerTest, portConsolidationAddPort) {
  PortID portId(0);
  // adds a port "behind the back of" PortManager
  auto saiId0 = addPort(portId, cfg::PortSpeed::TWENTYFIVEG);

  // loads the added port into SaiStore
  SaiStore::getInstance()->release();
  SaiStore::getInstance()->reload();

  // add a port with the same lanes through PortManager
  std::shared_ptr<Port> swPort = makePort(p0);
  auto saiId1 = saiManagerTable->portManager().addPort(swPort);

  checkPort(portId, saiId0, true);
  // expect it to return the existing port rather than create a new one
  EXPECT_EQ(saiId0, saiId1);
}

void checkCounterExport(
    const std::string& portName,
    ExpectExport expectExport) {
  for (auto statKey : HwPortFb303Stats::kPortStatKeys()) {
    switch (expectExport) {
      case ExpectExport::EXPORT:
        EXPECT_TRUE(facebook::fbData->getStatMap()->contains(
            HwPortFb303Stats::statName(statKey, portName)));
        break;
      case ExpectExport::NO_EXPORT:
        EXPECT_FALSE(facebook::fbData->getStatMap()->contains(
            HwPortFb303Stats::statName(statKey, portName)));
        break;
    }
  }
}

TEST_F(PortManagerTest, changePortNameAndCheckCounters) {
  std::shared_ptr<Port> swPort = makePort(p0);
  saiManagerTable->portManager().addPort(swPort);
  for (auto statKey : HwPortFb303Stats::kPortStatKeys()) {
    EXPECT_TRUE(facebook::fbData->getStatMap()->contains(
        HwPortFb303Stats::statName(statKey, swPort->getName())));
  }
  auto newPort = swPort->clone();
  newPort->setName("eth1/1/1");
  saiManagerTable->portManager().changePort(swPort, newPort);
  checkCounterExport(swPort->getName(), ExpectExport::NO_EXPORT);
  checkCounterExport(newPort->getName(), ExpectExport::EXPORT);
}

TEST_F(PortManagerTest, updateStats) {
  std::shared_ptr<Port> swPort = makePort(p0);
  auto saiId = saiManagerTable->portManager().addPort(swPort);
  checkPort(PortID(0), saiId, true);
  saiManagerTable->portManager().updateStats(swPort->getID());
  auto portStat =
      saiManagerTable->portManager().getLastPortStat(swPort->getID());
  for (auto statKey : HwPortFb303Stats::kPortStatKeys()) {
    EXPECT_EQ(
        portStat->getCounterLastIncrement(
            HwPortFb303Stats::statName(statKey, swPort->getName())),
        0);
  }
}

TEST_F(PortManagerTest, portDisableStopsCounterExport) {
  std::shared_ptr<Port> swPort = makePort(p0);
  CHECK(swPort->isEnabled());
  saiManagerTable->portManager().addPort(swPort);
  checkCounterExport(swPort->getName(), ExpectExport::EXPORT);
  auto newPort = swPort->clone();
  newPort->setAdminState(cfg::PortState::DISABLED);
  saiManagerTable->portManager().changePort(swPort, newPort);
  checkCounterExport(swPort->getName(), ExpectExport::NO_EXPORT);
}

TEST_F(PortManagerTest, portReenableRestartsCounterExport) {
  std::shared_ptr<Port> swPort = makePort(p0);
  CHECK(swPort->isEnabled());
  saiManagerTable->portManager().addPort(swPort);
  checkCounterExport(swPort->getName(), ExpectExport::EXPORT);
  auto newPort = swPort->clone();
  newPort->setAdminState(cfg::PortState::DISABLED);
  saiManagerTable->portManager().changePort(swPort, newPort);
  checkCounterExport(swPort->getName(), ExpectExport::NO_EXPORT);
  auto newNewPort = newPort->clone();
  newNewPort->setAdminState(cfg::PortState::ENABLED);
  saiManagerTable->portManager().changePort(newPort, newNewPort);
  checkCounterExport(swPort->getName(), ExpectExport::EXPORT);
}

TEST_F(PortManagerTest, collectStatsAfterPortDisable) {
  std::shared_ptr<Port> swPort = makePort(p0);
  CHECK(swPort->isEnabled());
  saiManagerTable->portManager().addPort(swPort);
  checkCounterExport(swPort->getName(), ExpectExport::EXPORT);
  auto newPort = swPort->clone();
  newPort->setAdminState(cfg::PortState::DISABLED);
  saiManagerTable->portManager().changePort(swPort, newPort);
  saiManagerTable->portManager().updateStats(swPort->getID());
  EXPECT_EQ(saiManagerTable->portManager().getPortStats().size(), 0);
  checkCounterExport(swPort->getName(), ExpectExport::NO_EXPORT);
}

TEST_F(PortManagerTest, subsumedPorts) {
  // Port P0 has a port ID 0 and only be configured with all speeds.
  checkSubsumedPorts(p0, cfg::PortSpeed::XG, {});
  checkSubsumedPorts(p0, cfg::PortSpeed::TWENTYFIVEG, {});
  checkSubsumedPorts(p0, cfg::PortSpeed::FIFTYG, {PortID(p0.id + 1)});
  std::vector<PortID> expectedPortList = {
      PortID(p0.id + 1), PortID(p0.id + 2), PortID(p0.id + 3)};
  checkSubsumedPorts(p0, cfg::PortSpeed::FORTYG, expectedPortList);
  checkSubsumedPorts(p0, cfg::PortSpeed::HUNDREDG, expectedPortList);

  // Port P1 has a port ID 10 and only be configured with 10, 25 and 50G modes.
  checkSubsumedPorts(p1, cfg::PortSpeed::XG, {});
  checkSubsumedPorts(p1, cfg::PortSpeed::TWENTYFIVEG, {});
  checkSubsumedPorts(p1, cfg::PortSpeed::FIFTYG, {PortID(p1.id + 1)});
}

TEST_F(PortManagerTest, getTransceiverID) {
  std::vector<uint16_t> controllingPorts = {0, 4, 24};
  std::vector<uint16_t> expectedTcvrIDs = {0, 1, 6};
  for (auto i = 0; i < controllingPorts.size(); i++) {
    for (auto lane = 0; lane < 4; lane++) {
      uint16_t port = controllingPorts[i] + lane;
      SaiPlatformPort* platformPort = saiPlatform->getPort(PortID(port));
      EXPECT_TRUE(platformPort);
      EXPECT_EQ(expectedTcvrIDs[i], platformPort->getTransceiverID().value());
    }
  }
}

TEST_F(PortManagerTest, attributesFromSwPort) {
  std::shared_ptr<Port> swPort = makePort(p0);
  auto& portMgr = saiManagerTable->portManager();
  portMgr.addPort(swPort);
  auto portHandle = portMgr.getPortHandle(PortID(0));
  auto attrs = portMgr.attributesFromSwPort(swPort);
  EXPECT_EQ(attrs, portHandle->port->attributes());
}

TEST_F(PortManagerTest, swPortFromAttributes) {
  std::shared_ptr<Port> swPort = makePort(p0);
  auto& portMgr = saiManagerTable->portManager();
  portMgr.addPort(swPort);
  auto attrs = portMgr.attributesFromSwPort(swPort);
  auto newPort = portMgr.swPortFromAttributes(attrs);
  EXPECT_EQ(attrs, portMgr.attributesFromSwPort(newPort));
}
