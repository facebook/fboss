/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/FabricReachabilityManager.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/SwitchIdScopeResolver.h"
#include "fboss/agent/state/DsfNodeMap.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/HwTestHandle.h"
#include "fboss/agent/test/TestUtils.h"
#include "fboss/agent/types.h"

#include <folly/logging/xlog.h>
#include <gtest/gtest.h>

namespace facebook::fboss {
class FabricReachabilityManagerTest : public ::testing::Test {
 public:
  void SetUp() override {
    auto config = testConfigA(cfg::SwitchType::VOQ);
    handle_ = createTestHandle(&config);
    // Create a separate instance of DsfSubscriber (vs
    // using one from SwSwitch) for ease of testing.
    fabricReachabilityManager_ = std::make_unique<FabricReachabilityManager>();
  }

  HwSwitchMatcher getScope(const std::shared_ptr<DsfNode>& node) const {
    return handle_->getSw()->getScopeResolver()->scope(node);
  }

  HwSwitchMatcher getScope(const std::shared_ptr<Port>& port) const {
    return handle_->getSw()->getScopeResolver()->scope(port);
  }

  cfg::DsfNode makeDsfNodeCfg(int64_t switchId, std::string name) {
    cfg::DsfNode dsfNodeCfg;
    dsfNodeCfg.switchId() = switchId;
    dsfNodeCfg.name() = name;
    return dsfNodeCfg;
  }

  std::shared_ptr<DsfNode> makeDsfNode(
      int64_t switchId,
      std::string name,
      cfg::AsicType asicType = cfg::AsicType::ASIC_TYPE_RAMON,
      PlatformType platformType = PlatformType::PLATFORM_MERU400BFU) {
    auto dsfNode = std::make_shared<DsfNode>(SwitchID(switchId));
    auto cfgDsfNode = makeDsfNodeCfg(switchId, name);
    cfgDsfNode.asicType() = asicType;
    cfgDsfNode.platformType() = platformType;
    cfgDsfNode.type() = cfg::DsfNodeType::FABRIC_NODE;
    dsfNode->fromThrift(cfgDsfNode);
    return dsfNode;
  }

  std::shared_ptr<Port> makePort(const int portId) {
    state::PortFields portFields;
    portFields.portId() = portId;
    portFields.portName() = folly::sformat("port{}", portId);
    auto swPort = std::make_shared<Port>(std::move(portFields));
    swPort->setAdminState(cfg::PortState::ENABLED);
    swPort->setProfileId(
        cfg::PortProfileID::PROFILE_53POINT125G_1_PAM4_RS545_COPPER);
    return swPort;
  }

  std::vector<cfg::PortNeighbor> createPortNeighbor(
      std::string portName,
      std::string switchName) {
    cfg::PortNeighbor nbr;
    nbr.remotePort() = portName;
    nbr.remoteSystem() = switchName;
    return {nbr};
  }

 protected:
  std::unique_ptr<HwTestHandle> handle_;
  std::unique_ptr<FabricReachabilityManager> fabricReachabilityManager_;
};

TEST_F(FabricReachabilityManagerTest, validateRemoteOffset) {
  auto oldState = std::make_shared<SwitchState>();
  auto newState = std::make_shared<SwitchState>();

  // create port with neighbor reachability
  std::shared_ptr<Port> swPort = makePort(1);
  swPort->setExpectedNeighborReachability(
      createPortNeighbor("fab1/9/2", "rdswA"));
  newState->getPorts()->addNode(swPort, getScope(swPort));

  std::map<PortID, FabricEndpoint> hwReachabilityMap;
  FabricEndpoint endpoint;
  endpoint.portId() =
      160; // known from platforom mapping for Meru400biuPlatformMapping
  endpoint.switchId() = 10;
  endpoint.isAttached() = true;
  hwReachabilityMap.emplace(swPort->getID(), endpoint);

  auto dsfNode = makeDsfNode(
      10,
      "rdswA",
      cfg::AsicType::ASIC_TYPE_JERICHO2,
      PlatformType::PLATFORM_MERU400BIU);
  auto dsfNodeMap = std::make_shared<MultiSwitchDsfNodeMap>();
  dsfNodeMap->addNode(dsfNode, getScope(dsfNode));
  newState->resetDsfNodes(dsfNodeMap);

  StateDelta delta(oldState, newState);
  fabricReachabilityManager_->stateUpdated(delta);

  const auto expectedReachabilityMap =
      fabricReachabilityManager_->processReachabilityInfo(hwReachabilityMap);
  EXPECT_EQ(expectedReachabilityMap.size(), 1);

  for (const auto& expectedReachability : expectedReachabilityMap) {
    const auto& neighbor = expectedReachability.second;
    // remote offset for jericho2 is 256
    EXPECT_EQ(neighbor.expectedPortId(), 160 + 256);
    EXPECT_EQ(neighbor.expectedPortName(), "fab1/9/2");
    EXPECT_EQ(*neighbor.expectedPortId(), *neighbor.portId());
    EXPECT_EQ(neighbor.expectedPortName(), neighbor.portName());
  }
}

TEST_F(FabricReachabilityManagerTest, validateProcessReachabilityInfo) {
  auto oldState = std::make_shared<SwitchState>();
  auto newState = std::make_shared<SwitchState>();

  // create port with neighbor reachability
  std::shared_ptr<Port> swPort = makePort(1);
  swPort->setExpectedNeighborReachability(
      createPortNeighbor("fab1/2/4", "fdswA"));
  newState->getPorts()->addNode(swPort, getScope(swPort));

  std::map<PortID, FabricEndpoint> hwReachabilityMap;
  FabricEndpoint endpoint;
  endpoint.portId() = 79; // known from platforom mapping for ramon
  endpoint.switchId() = 10;
  endpoint.isAttached() = true;

  hwReachabilityMap.emplace(swPort->getID(), endpoint);

  auto dsfNode = makeDsfNode(10, "fdswA", cfg::AsicType::ASIC_TYPE_RAMON);
  auto dsfNodeMap = std::make_shared<MultiSwitchDsfNodeMap>();
  dsfNodeMap->addNode(dsfNode, getScope(dsfNode));
  newState->resetDsfNodes(dsfNodeMap);

  StateDelta delta(oldState, newState);
  fabricReachabilityManager_->stateUpdated(delta);

  const auto expectedReachabilityMap =
      fabricReachabilityManager_->processReachabilityInfo(hwReachabilityMap);
  EXPECT_EQ(expectedReachabilityMap.size(), 1);

  for (const auto& expectedReachability : expectedReachabilityMap) {
    const auto& neighbor = expectedReachability.second;
    EXPECT_EQ(neighbor.expectedPortId(), 79);
    EXPECT_EQ(neighbor.expectedSwitchId(), 10);
    EXPECT_EQ(neighbor.switchId(), 10);
    EXPECT_EQ(neighbor.expectedSwitchName(), "fdswA");
    EXPECT_EQ(neighbor.expectedPortName(), "fab1/2/4");
    EXPECT_TRUE(*neighbor.isAttached());
    EXPECT_EQ(*neighbor.expectedPortId(), *neighbor.portId());
    EXPECT_EQ(neighbor.expectedPortName(), neighbor.portName());
  }
}

TEST_F(FabricReachabilityManagerTest, validateUnattachedEndpoint) {
  auto oldState = std::make_shared<SwitchState>();
  auto newState = std::make_shared<SwitchState>();

  // create port with neighbor reachability
  std::shared_ptr<Port> swPort = makePort(1);
  swPort->setExpectedNeighborReachability(
      createPortNeighbor("fab1/2/4", "fdswA"));
  newState->getPorts()->addNode(swPort, getScope(swPort));

  auto dsfNode = makeDsfNode(10, "fdswA");
  auto dsfNodeMap = std::make_shared<MultiSwitchDsfNodeMap>();
  dsfNodeMap->addNode(dsfNode, getScope(dsfNode));
  newState->resetDsfNodes(dsfNodeMap);

  std::map<PortID, FabricEndpoint> hwReachabilityMap;
  FabricEndpoint endpoint;
  // dont set anything in the endpoint
  endpoint.isAttached() = false;
  hwReachabilityMap.emplace(swPort->getID(), endpoint);

  // update
  StateDelta delta(oldState, newState);
  fabricReachabilityManager_->stateUpdated(delta);

  const auto expectedReachabilityMap =
      fabricReachabilityManager_->processReachabilityInfo(hwReachabilityMap);
  EXPECT_EQ(expectedReachabilityMap.size(), 1);

  // when unattached, we can't get get expectedPortId
  for (const auto& expectedReachability : expectedReachabilityMap) {
    const auto& neighbor = expectedReachability.second;
    EXPECT_EQ(neighbor.expectedSwitchId(), 10);
    EXPECT_EQ(*neighbor.expectedSwitchName(), "fdswA");
    EXPECT_EQ(*neighbor.expectedPortName(), "fab1/2/4");
    EXPECT_TRUE(neighbor.expectedPortId().has_value());

    EXPECT_FALSE(neighbor.portName().has_value());
    EXPECT_FALSE(neighbor.switchName().has_value());
    EXPECT_FALSE(*neighbor.isAttached());
  }
}

TEST_F(FabricReachabilityManagerTest, validateUnexpectedNeighbors) {
  auto oldState = std::make_shared<SwitchState>();
  auto newState = std::make_shared<SwitchState>();

  // create port with neighbor reachability
  std::shared_ptr<Port> swPort = makePort(1);
  swPort->setExpectedNeighborReachability(
      createPortNeighbor("fab1/2/3", "fdswA"));
  newState->getPorts()->addNode(swPort, getScope(swPort));

  std::map<PortID, FabricEndpoint> hwReachabilityMap;
  FabricEndpoint endpoint;
  endpoint.switchId() = 10;
  endpoint.isAttached() = true;
  endpoint.portId() = 79;
  hwReachabilityMap.emplace(swPort->getID(), endpoint);

  auto dsfNode1 = makeDsfNode(10, "fdswB");
  auto dsfNode2 = makeDsfNode(20, "fdswA");

  auto dsfNodeMap = std::make_shared<MultiSwitchDsfNodeMap>();
  dsfNodeMap->addNode(dsfNode1, getScope(dsfNode1));
  dsfNodeMap->addNode(dsfNode2, getScope(dsfNode2));
  newState->resetDsfNodes(dsfNodeMap);

  // update
  StateDelta delta(oldState, newState);
  fabricReachabilityManager_->stateUpdated(delta);

  const auto expectedReachabilityMap =
      fabricReachabilityManager_->processReachabilityInfo(hwReachabilityMap);

  for (const auto& expectedReachability : expectedReachabilityMap) {
    const auto& neighbor = expectedReachability.second;
    EXPECT_EQ(expectedReachabilityMap.size(), 1);
    EXPECT_NE(*neighbor.expectedSwitchId(), *neighbor.switchId());
    EXPECT_EQ(neighbor.expectedPortName(), "fab1/2/3");
    EXPECT_NE(neighbor.expectedSwitchName(), neighbor.switchName());
    EXPECT_NE(neighbor.expectedPortName(), neighbor.portName());
    EXPECT_TRUE(neighbor.expectedPortId().has_value());
    // portId = 79 results in fab1/2/4
    EXPECT_EQ(neighbor.portName(), "fab1/2/4");
  }
}

} // namespace facebook::fboss
