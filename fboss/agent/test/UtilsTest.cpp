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
#include "fboss/agent/AgentFeatures.h"
#include "fboss/agent/FbossError.h"
#include "fboss/agent/HwAsicTable.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/SwitchSettings.h"
#include "fboss/agent/state/SwitchState.h"
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
  auto hwAsicTable = sw->getHwAsicTable();
  auto hwAsic = hwAsicTable->getHwAsicIf(SwitchID(0));
  utility::addTrapPacketAcl(
      hwAsic, &config, folly::CIDRNetwork{"10.0.0.1", 32});
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

TEST_F(UtilsTest, coveringSysPortRange) {
  cfg::SwitchInfo switchInfo;
  cfg::Range64 lowerRange, higherRange;
  lowerRange.minimum() = 100;
  lowerRange.maximum() = 200;
  higherRange.minimum() = 16100;
  higherRange.maximum() = 16200;
  cfg::SystemPortRanges ranges;
  ranges.systemPortRanges()->push_back(lowerRange);
  ranges.systemPortRanges()->push_back(higherRange);
  switchInfo.systemPortRanges() = ranges;
  std::map<int64_t, cfg::SwitchInfo> switchIdToInfo{{1, switchInfo}};
  auto expectInLowerRange = [&](int64_t id) {
    EXPECT_EQ(
        getCoveringSysPortRange(InterfaceID(id), switchIdToInfo), lowerRange);

    EXPECT_EQ(
        getCoveringSysPortRange(SystemPortID(id), switchIdToInfo), lowerRange);
  };
  auto expectInHigherRange = [&](int64_t id) {
    EXPECT_EQ(
        getCoveringSysPortRange(InterfaceID(id), switchIdToInfo), higherRange);

    EXPECT_EQ(
        getCoveringSysPortRange(SystemPortID(id), switchIdToInfo), higherRange);
  };
  auto expectThrow = [&](int64_t id) {
    EXPECT_THROW(
        getCoveringSysPortRange(InterfaceID(id), switchIdToInfo), FbossError);

    EXPECT_THROW(
        getCoveringSysPortRange(SystemPortID(id), switchIdToInfo), FbossError);
  };
  expectInLowerRange(100);
  expectInLowerRange(110);
  expectInLowerRange(200);
  expectThrow(99);
  expectThrow(299);
  expectInHigherRange(16100);
  expectInHigherRange(16110);
  expectInHigherRange(16200);
  expectThrow(15099);
  expectThrow(17299);
}

TEST_F(UtilsTest, getPortIDForRelocatedFabricPort) {
  // getPortID(sysPortId, switchId, state) is the reverse lookup used by fabric
  // link monitoring to map a system port back to its port. With
  // fabric_ports_uniform_local_offset enabled, relocated fabric ports own a
  // getSystemPortID-based system port and must resolve through this path;
  // without the flag fabric ports use the legacy offset scheme and are skipped.
  auto switchId = SwitchID(kVoqSwitchIdBegin);
  auto matcher = HwSwitchMatcher(std::unordered_set<SwitchID>({switchId}));

  // portIdRange minimum 0, localSystemPortOffset = sysPortMin = 100, so the
  // relocated fabric port at 32777 maps to system port 32777 + 100 - 0 = 32877.
  auto switchInfo = createSwitchInfo(
      cfg::SwitchType::VOQ,
      cfg::AsicType::ASIC_TYPE_MOCK,
      0, /* portIdMin */
      65535, /* portIdMax */
      0, /* switchIndex */
      100, /* sysPortMin */
      1100 /* sysPortMax */);
  auto switchSettings = std::make_shared<SwitchSettings>();
  switchSettings->setSwitchIdToSwitchInfo({{switchId, switchInfo}});

  auto state = std::make_shared<SwitchState>();
  addSwitchSettingsToState(state, switchSettings, switchId);
  const std::string fabricPortName = "fab1/1/1";
  auto fabricPort = std::make_shared<Port>(PortID(32777), fabricPortName);
  fabricPort->setPortType(cfg::PortType::FABRIC_PORT);
  fabricPort->setScope(cfg::Scope::LOCAL);
  state->getPorts()->addNode(std::move(fabricPort), matcher);
  state->publish();

  auto sysPortId = getSystemPortID(
      PortID(32777),
      cfg::Scope::LOCAL,
      switchSettings->getSwitchIdToSwitchInfo(),
      switchId);
  EXPECT_EQ(sysPortId, SystemPortID(32877));

  FLAGS_fabric_ports_uniform_local_offset = true;
  EXPECT_EQ(getPortID(sysPortId, switchId, state), PortID(32777));

  // With relocation off, fabric ports are skipped, so the reverse lookup finds
  // no matching port and throws rather than mis-resolving.
  FLAGS_fabric_ports_uniform_local_offset = false;
  EXPECT_THROW(getPortID(sysPortId, switchId, state), FbossError);
}
