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
    ManagerTestBase::SetUp();
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
};

TEST_F(PortManagerTest, addPort) {
  auto saiId = addPort(42, true);
  checkPort(PortID(42), saiId, true);
}

TEST_F(PortManagerTest, addTwoPorts) {
  addPort(42, true);
  auto saiId2 = addPort(43, false);
  checkPort(PortID(43), saiId2, false);
}

TEST_F(PortManagerTest, addDupIdPorts) {
  addPort(42, true);
  EXPECT_THROW(addPort(42, false), FbossError);
}

TEST_F(PortManagerTest, getBySwId) {
  addPort(42, true);
  SaiPort* port = saiManagerTable->portManager().getPort(PortID(42));
  EXPECT_TRUE(port);
  EXPECT_EQ(port->attributes().adminState, true);
  EXPECT_EQ(port->attributes().speed, 25000);
  EXPECT_EQ(port->attributes().hwLaneList.size(), 1);
  EXPECT_EQ(port->attributes().hwLaneList[0], 42);
}

TEST_F(PortManagerTest, getNonExistent) {
  addPort(42, true);
  SaiPort* port = saiManagerTable->portManager().getPort(PortID(43));
  EXPECT_FALSE(port);
}

TEST_F(PortManagerTest, removePort) {
  addPort(42, true);
  SaiPort* port = saiManagerTable->portManager().getPort(PortID(42));
  EXPECT_TRUE(port);
  saiManagerTable->portManager().removePort(PortID(42));
  port = saiManagerTable->portManager().getPort(PortID(42));
  EXPECT_FALSE(port);
}

TEST_F(PortManagerTest, removeNonExistentPort) {
  addPort(42, true);
  SaiPort* port = saiManagerTable->portManager().getPort(PortID(42));
  EXPECT_TRUE(port);
  EXPECT_THROW(
      saiManagerTable->portManager().removePort(PortID(43)), FbossError);
}

TEST_F(PortManagerTest, changePortAdminState) {
  auto saiId = addPort(42, true);
  auto swPort = std::make_shared<Port>(PortID(42), std::string("port42"));
  swPort->setSpeed(cfg::PortSpeed::TWENTYFIVEG);
  swPort->setAdminState(cfg::PortState::DISABLED);
  saiManagerTable->portManager().changePort(swPort);
  checkPort(swPort->getID(), saiId, false);
}

TEST_F(PortManagerTest, changePortNoChange) {
  auto saiId = addPort(42, true);
  auto swPort = std::make_shared<Port>(PortID(42), std::string("port42"));
  swPort->setSpeed(cfg::PortSpeed::TWENTYFIVEG);
  swPort->setAdminState(cfg::PortState::ENABLED);
  saiManagerTable->portManager().changePort(swPort);
  checkPort(swPort->getID(), saiId, true);
}

TEST_F(PortManagerTest, changeNonExistentPort) {
  addPort(42, true);
  auto swPort = std::make_shared<Port>(PortID(43), std::string("port42"));
  swPort->setSpeed(cfg::PortSpeed::TWENTYFIVEG);
  EXPECT_THROW(saiManagerTable->portManager().changePort(swPort), FbossError);
}
