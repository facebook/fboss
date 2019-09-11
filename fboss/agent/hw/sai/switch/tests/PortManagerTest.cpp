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
#include "fboss/agent/hw/sai/api/SaiApiTable.h"
#include "fboss/agent/hw/sai/fake/FakeSai.h"
#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiPortManager.h"
#include "fboss/agent/hw/sai/switch/tests/ManagerTestBase.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/types.h"

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
  void checkPort(const PortID& swId, sai_object_id_t saiId, bool enabled) {
    // Check SaiPortApi perspective
    auto& portApi = saiApiTable->portApi();
    SaiPortTraits::Attributes::AdminState adminStateAttribute;
    SaiPortTraits::Attributes::HwLaneList hwLaneListAttribute;
    SaiPortTraits::Attributes::Speed speedAttribute;
    SaiPortTraits::Attributes::FecMode fecMode;
    SaiPortTraits::Attributes::InternalLoopbackMode ilbMode;
    auto gotAdminState = portApi.getAttribute(adminStateAttribute, saiId);
    EXPECT_EQ(enabled, gotAdminState);
    auto gotLanes = portApi.getAttribute(hwLaneListAttribute, saiId);
    EXPECT_EQ(1, gotLanes.size());
    EXPECT_EQ(swId, gotLanes[0]);
    auto gotSpeed = portApi.getAttribute(speedAttribute, saiId);
    EXPECT_EQ(25000, gotSpeed);
    auto gotFecMode = portApi.getAttribute(fecMode, saiId);
    EXPECT_EQ(static_cast<int32_t>(SAI_PORT_FEC_MODE_NONE), gotFecMode);
    auto gotIlbMode = portApi.getAttribute(ilbMode, saiId);
    EXPECT_EQ(
        static_cast<int32_t>(SAI_PORT_INTERNAL_LOOPBACK_MODE_NONE), gotIlbMode);
  }

  /**
   * DO NOT use this routine for adding ports.
   * This is only used to verify port consolidation logic in sai port
   * manager. It makes certain assumptions about the lane numbers and
   * also adds the port under the hood bypassing the port manager.
   */
  sai_object_id_t addPort(const PortID& swId, cfg::PortSpeed portSpeed) {
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
                                      std::nullopt};
    return portApi.create2<SaiPortTraits>(a, 0);
  }

  TestPort p0;
  TestPort p1;
};

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
  saiManagerTable->portManager().removePort(PortID(0));
  port = saiManagerTable->portManager().getPortHandle(PortID(0));
  EXPECT_FALSE(port);
}

TEST_F(PortManagerTest, removeNonExistentPort) {
  std::shared_ptr<Port> swPort = makePort(p0);
  saiManagerTable->portManager().addPort(swPort);
  SaiPortHandle* port = saiManagerTable->portManager().getPortHandle(PortID(0));
  EXPECT_TRUE(port);
  EXPECT_THROW(
      saiManagerTable->portManager().removePort(PortID(10)), FbossError);
}

TEST_F(PortManagerTest, changePortAdminState) {
  std::shared_ptr<Port> swPort = makePort(p0);
  auto saiId = saiManagerTable->portManager().addPort(swPort);
  swPort->setAdminState(cfg::PortState::DISABLED);
  saiManagerTable->portManager().changePort(swPort);
  checkPort(swPort->getID(), saiId, false);
}

TEST_F(PortManagerTest, changePortNoChange) {
  std::shared_ptr<Port> swPort = makePort(p0);
  auto saiId = saiManagerTable->portManager().addPort(swPort);
  swPort->setSpeed(cfg::PortSpeed::TWENTYFIVEG);
  swPort->setAdminState(cfg::PortState::ENABLED);
  saiManagerTable->portManager().changePort(swPort);
  checkPort(swPort->getID(), saiId, true);
}

TEST_F(PortManagerTest, changeNonExistentPort) {
  std::shared_ptr<Port> swPort = makePort(p0);
  std::shared_ptr<Port> swPort2 = makePort(p1);
  saiManagerTable->portManager().addPort(swPort);
  swPort2->setSpeed(cfg::PortSpeed::TWENTYFIVEG);
  EXPECT_THROW(saiManagerTable->portManager().changePort(swPort2), FbossError);
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
