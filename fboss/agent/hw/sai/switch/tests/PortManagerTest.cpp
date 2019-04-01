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
  void checkPort(const PortID& swId, sai_object_id_t saiId, bool enabled) {
    // Check SaiPortApi perspective
    auto& portApi = saiApiTable->portApi();
    PortApiParameters::Attributes::AdminState adminStateAttribute;
    std::vector<uint32_t> ls;
    ls.resize(1);
    PortApiParameters::Attributes::HwLaneList hwLaneListAttribute(ls);
    PortApiParameters::Attributes::Speed speedAttribute;
    auto gotAdminState = portApi.getAttribute(adminStateAttribute, saiId);
    EXPECT_EQ(enabled, gotAdminState);
    auto gotLanes = portApi.getAttribute(hwLaneListAttribute, saiId);
    EXPECT_EQ(1, gotLanes.size());
    EXPECT_EQ(swId, gotLanes[0]);
    auto gotSpeed = portApi.getAttribute(speedAttribute, saiId);
    EXPECT_EQ(25000, gotSpeed);
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
  SaiPort* port = saiManagerTable->portManager().getPort(PortID(10));
  EXPECT_TRUE(port);
  EXPECT_EQ(port->attributes().adminState, true);
  EXPECT_EQ(port->attributes().speed, 25000);
  EXPECT_EQ(port->attributes().hwLaneList.size(), 1);
  EXPECT_EQ(port->attributes().hwLaneList[0], 10);
}

TEST_F(PortManagerTest, getNonExistent) {
  std::shared_ptr<Port> swPort = makePort(p0);
  saiManagerTable->portManager().addPort(swPort);
  SaiPort* port = saiManagerTable->portManager().getPort(PortID(10));
  EXPECT_FALSE(port);
}

TEST_F(PortManagerTest, removePort) {
  std::shared_ptr<Port> swPort = makePort(p0);
  saiManagerTable->portManager().addPort(swPort);
  SaiPort* port = saiManagerTable->portManager().getPort(PortID(0));
  EXPECT_TRUE(port);
  saiManagerTable->portManager().removePort(PortID(0));
  port = saiManagerTable->portManager().getPort(PortID(0));
  EXPECT_FALSE(port);
}

TEST_F(PortManagerTest, removeNonExistentPort) {
  std::shared_ptr<Port> swPort = makePort(p0);
  saiManagerTable->portManager().addPort(swPort);
  SaiPort* port = saiManagerTable->portManager().getPort(PortID(0));
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
