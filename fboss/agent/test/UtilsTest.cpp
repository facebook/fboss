/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/Utils.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/test/HwTestHandle.h"
#include "fboss/agent/test/TestUtils.h"
#include "fboss/agent/test/utils/TrapPacketUtils.h"

using namespace facebook::fboss;
class UtilsTest : public ::testing::Test {
 public:
  void SetUp() override {
    // Setup a default state object
    auto config = testConfigA();
    handle = createTestHandle(&config);
    sw = handle->getSw();
  }
  SwSwitch* sw{nullptr};
  std::unique_ptr<HwTestHandle> handle{nullptr};
};

TEST_F(UtilsTest, getIP) {
  EXPECT_NO_THROW(getAnyIntfIP(sw->getState()));
  EXPECT_NO_THROW(getSwitchIntfIP(sw->getState(), InterfaceID(1)));
}

TEST_F(UtilsTest, getIPv6) {
  EXPECT_NO_THROW(getAnyIntfIPv6(sw->getState()));
  EXPECT_NO_THROW(getSwitchIntfIPv6(sw->getState(), InterfaceID(1)));
}

TEST_F(UtilsTest, getPortsForInterface) {
  EXPECT_GT(getPortsForInterface(InterfaceID(1), sw->getState()).size(), 0);
}

TEST_F(UtilsTest, AddTrapPacketAcl) {
  auto state = sw->getState();
  auto config = testConfigA();
  utility::addTrapPacketAcl(&config, folly::CIDRNetwork{"10.0.0.1", 128});
  sw->applyConfig("AddTrapPacketAcl", config);
  EXPECT_NE(sw->getState()->getAcls()->getNodeIf("trap-10.0.0.1"), nullptr);
}

TEST_F(UtilsTest, numFabricLevelsNoDsfNodes) {
  EXPECT_EQ(numFabricLevels({}), 0);
}

TEST_F(UtilsTest, numFabricLevelsNoFabricNodes) {
  std::map<int64_t, cfg::DsfNode> dsfNodes;
  dsfNodes.insert({0, makeDsfNodeCfg(0, cfg::DsfNodeType::INTERFACE_NODE)});
  dsfNodes.insert({4, makeDsfNodeCfg(4, cfg::DsfNodeType::INTERFACE_NODE)});
  EXPECT_EQ(numFabricLevels(dsfNodes), 0);
}

TEST_F(UtilsTest, numFabricLevelsL1DsfNoFabricLevelSet) {
  std::map<int64_t, cfg::DsfNode> dsfNodes;
  dsfNodes.insert({0, makeDsfNodeCfg(0, cfg::DsfNodeType::INTERFACE_NODE)});
  dsfNodes.insert({4, makeDsfNodeCfg(4, cfg::DsfNodeType::FABRIC_NODE)});
  EXPECT_EQ(numFabricLevels(dsfNodes), 1);
}

TEST_F(UtilsTest, numFabricLevelsL1Dsf) {
  std::map<int64_t, cfg::DsfNode> dsfNodes;
  dsfNodes.insert({0, makeDsfNodeCfg(0, cfg::DsfNodeType::INTERFACE_NODE)});
  dsfNodes.insert(
      {4,
       makeDsfNodeCfg(
           4,
           cfg::DsfNodeType::FABRIC_NODE,
           1,
           cfg::AsicType::ASIC_TYPE_MOCK,
           1)});
  EXPECT_EQ(numFabricLevels(dsfNodes), 1);
}

TEST_F(UtilsTest, numFabricLevelsL2Dsf) {
  std::map<int64_t, cfg::DsfNode> dsfNodes;
  dsfNodes.insert({0, makeDsfNodeCfg(0, cfg::DsfNodeType::INTERFACE_NODE)});
  dsfNodes.insert(
      {4,
       makeDsfNodeCfg(
           4,
           cfg::DsfNodeType::FABRIC_NODE,
           1,
           cfg::AsicType::ASIC_TYPE_MOCK,
           1)});
  dsfNodes.insert(
      {6,
       makeDsfNodeCfg(
           6,
           cfg::DsfNodeType::FABRIC_NODE,
           std::nullopt,
           cfg::AsicType::ASIC_TYPE_MOCK,
           2)});
  EXPECT_EQ(numFabricLevels(dsfNodes), 2);
}
