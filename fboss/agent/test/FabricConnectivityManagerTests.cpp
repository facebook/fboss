/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/FabricConnectivityManager.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/SwitchIdScopeResolver.h"
#include "fboss/agent/state/DsfNodeMap.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/HwTestHandle.h"
#include "fboss/agent/test/TestUtils.h"
#include "fboss/agent/types.h"

#include <gtest/gtest.h>

namespace facebook::fboss {
class FabricConnectivityManagerTest : public ::testing::Test {
 public:
  void SetUp() override {
    auto config = testConfigA(cfg::SwitchType::VOQ);
    handle_ = createTestHandle(&config);
    // Create a separate instance of DsfSubscriber (vs
    // using one from SwSwitch) for ease of testing.
    fabricConnectivityManager_ = std::make_unique<FabricConnectivityManager>();
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
    cfgDsfNode.nodeMac() = "02:00:00:00:0F:0B";
    dsfNode->fromThrift(cfgDsfNode);
    return dsfNode;
  }

  std::shared_ptr<Port> makePort(
      const int portId,
      const cfg::PortType portType = cfg::PortType::FABRIC_PORT) {
    state::PortFields portFields;
    portFields.portId() = portId;
    portFields.portName() = folly::sformat("port{}", portId);
    auto swPort = std::make_shared<Port>(std::move(portFields));
    swPort->setAdminState(cfg::PortState::ENABLED);
    swPort->setProfileId(
        cfg::PortProfileID::PROFILE_53POINT125G_1_PAM4_RS545_COPPER);
    swPort->setPortType(portType);
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
  std::unique_ptr<FabricConnectivityManager> fabricConnectivityManager_;
};

TEST_F(FabricConnectivityManagerTest, validateRemoteOffset) {
  auto oldState = std::make_shared<SwitchState>();
  auto newState = std::make_shared<SwitchState>();

  // create port with neighbor connectivity
  std::shared_ptr<Port> swPort = makePort(1);
  swPort->setExpectedNeighborReachability(
      createPortNeighbor("fab1/9/2", "rdswA"));
  newState->getPorts()->addNode(swPort, getScope(swPort));

  std::map<PortID, FabricEndpoint> hwConnectivityMap;
  FabricEndpoint endpoint;
  endpoint.portId() =
      160; // known from platforom mapping for Meru400biuPlatformMapping
  endpoint.switchId() = 10;
  endpoint.isAttached() = true;
  hwConnectivityMap.emplace(swPort->getID(), endpoint);

  auto dsfNode = makeDsfNode(
      10,
      "rdswA",
      cfg::AsicType::ASIC_TYPE_JERICHO2,
      PlatformType::PLATFORM_MERU400BIU);
  auto dsfNodeMap = std::make_shared<MultiSwitchDsfNodeMap>();
  dsfNodeMap->addNode(dsfNode, getScope(dsfNode));
  newState->resetDsfNodes(dsfNodeMap);

  StateDelta delta(oldState, newState);
  fabricConnectivityManager_->stateUpdated(delta);

  const auto expectedConnectivityMap =
      fabricConnectivityManager_->processConnectivityInfo(hwConnectivityMap);
  EXPECT_EQ(expectedConnectivityMap.size(), 1);

  for (const auto& expectedConnectivity : expectedConnectivityMap) {
    const auto& neighbor = expectedConnectivity.second;
    // remote offset for jericho2 is 256
    EXPECT_EQ(neighbor.expectedPortId(), 160);
    EXPECT_EQ(neighbor.expectedPortName(), "fab1/9/2");
    EXPECT_EQ(*neighbor.portId(), 160);
    XLOG(ERR) << "foo_bar expected: " << neighbor.expectedPortName().value()
              << " actual: " << neighbor.portName().value();
    EXPECT_EQ(neighbor.expectedPortName(), neighbor.portName());
  }
}

TEST_F(FabricConnectivityManagerTest, validateProcessConnectivityInfo) {
  auto oldState = std::make_shared<SwitchState>();
  auto newState = std::make_shared<SwitchState>();

  // create port with neighbor connectivity
  std::shared_ptr<Port> swPort = makePort(1);
  swPort->setExpectedNeighborReachability(
      createPortNeighbor("fab1/2/4", "fdswA"));
  newState->getPorts()->addNode(swPort, getScope(swPort));

  std::map<PortID, FabricEndpoint> hwConnectivityMap;
  FabricEndpoint endpoint;
  endpoint.portId() = 79; // known from platforom mapping for ramon
  endpoint.switchId() = 10;
  endpoint.isAttached() = true;

  hwConnectivityMap.emplace(swPort->getID(), endpoint);

  auto dsfNode = makeDsfNode(10, "fdswA", cfg::AsicType::ASIC_TYPE_RAMON);
  auto dsfNodeMap = std::make_shared<MultiSwitchDsfNodeMap>();
  dsfNodeMap->addNode(dsfNode, getScope(dsfNode));
  newState->resetDsfNodes(dsfNodeMap);

  StateDelta delta(oldState, newState);
  fabricConnectivityManager_->stateUpdated(delta);

  const auto expectedConnectivityMap =
      fabricConnectivityManager_->processConnectivityInfo(hwConnectivityMap);
  EXPECT_EQ(expectedConnectivityMap.size(), 1);

  for (const auto& expectedConnectivity : expectedConnectivityMap) {
    const auto& neighbor = expectedConnectivity.second;
    EXPECT_EQ(neighbor.expectedPortId(), 79);
    EXPECT_EQ(neighbor.expectedSwitchId(), 10);
    EXPECT_EQ(neighbor.switchId(), 10);
    EXPECT_EQ(neighbor.expectedSwitchName(), "fdswA");
    EXPECT_EQ(neighbor.expectedPortName(), "fab1/2/4");
    EXPECT_TRUE(*neighbor.isAttached());
    EXPECT_EQ(*neighbor.expectedPortId(), *neighbor.portId());
    EXPECT_EQ(neighbor.expectedPortName(), neighbor.portName());
  }

  EXPECT_FALSE(
      fabricConnectivityManager_->isConnectivityInfoMissing(PortID(1)));
  EXPECT_FALSE(
      fabricConnectivityManager_->isConnectivityInfoMismatch(PortID(1)));
}

TEST_F(FabricConnectivityManagerTest, validateUnattachedEndpoint) {
  auto oldState = std::make_shared<SwitchState>();
  auto newState = std::make_shared<SwitchState>();

  // create port with neighbor connectivity
  std::shared_ptr<Port> swPort = makePort(1);
  swPort->setExpectedNeighborReachability(
      createPortNeighbor("fab1/2/4", "fdswA"));
  newState->getPorts()->addNode(swPort, getScope(swPort));

  auto dsfNode = makeDsfNode(10, "fdswA");
  auto dsfNodeMap = std::make_shared<MultiSwitchDsfNodeMap>();
  dsfNodeMap->addNode(dsfNode, getScope(dsfNode));
  newState->resetDsfNodes(dsfNodeMap);

  std::map<PortID, FabricEndpoint> hwConnectivityMap;
  FabricEndpoint endpoint;
  // dont set anything in the endpoint
  endpoint.isAttached() = false;
  hwConnectivityMap.emplace(swPort->getID(), endpoint);

  // update
  StateDelta delta(oldState, newState);
  fabricConnectivityManager_->stateUpdated(delta);

  const auto expectedConnectivityMap =
      fabricConnectivityManager_->processConnectivityInfo(hwConnectivityMap);
  EXPECT_EQ(expectedConnectivityMap.size(), 1);

  // when unattached, we can't get get expectedPortId
  for (const auto& expectedConnectivity : expectedConnectivityMap) {
    const auto& neighbor = expectedConnectivity.second;
    EXPECT_EQ(neighbor.expectedSwitchId(), 10);
    EXPECT_EQ(*neighbor.expectedSwitchName(), "fdswA");
    EXPECT_EQ(*neighbor.expectedPortName(), "fab1/2/4");
    EXPECT_TRUE(neighbor.expectedPortId().has_value());

    EXPECT_FALSE(neighbor.portName().has_value());
    EXPECT_FALSE(neighbor.switchName().has_value());
    EXPECT_FALSE(*neighbor.isAttached());
  }

  EXPECT_FALSE(
      fabricConnectivityManager_->isConnectivityInfoMissing(PortID(1)));
  // unattached port implies connectivity issues
  EXPECT_TRUE(
      fabricConnectivityManager_->isConnectivityInfoMismatch(PortID(1)));

  // another case for unattached endpoint is where its not having
  // neighbor connectivity cfg as well ..this is case of down fabric ports
  // and should be discounted
  auto newState2 = std::make_shared<SwitchState>();
  std::shared_ptr<Port> swPort2 = makePort(2);
  newState->getPorts()->addNode(swPort2, getScope(swPort2));
  FabricEndpoint endpoint2;
  // dont set anything in the endpoint
  endpoint2.isAttached() = false;
  hwConnectivityMap.emplace(swPort2->getID(), endpoint2);

  // update
  StateDelta delta2(newState, newState2);
  fabricConnectivityManager_->stateUpdated(delta2);

  EXPECT_FALSE(
      fabricConnectivityManager_->isConnectivityInfoMismatch(PortID(2)));
}

TEST_F(FabricConnectivityManagerTest, validateUnexpectedNeighbors) {
  auto oldState = std::make_shared<SwitchState>();
  auto newState = std::make_shared<SwitchState>();

  // create port with neighbor connectivity
  std::shared_ptr<Port> swPort = makePort(1);
  swPort->setExpectedNeighborReachability(
      createPortNeighbor("fab1/2/3", "fdswA"));
  newState->getPorts()->addNode(swPort, getScope(swPort));

  std::map<PortID, FabricEndpoint> hwConnectivityMap;
  FabricEndpoint endpoint;
  endpoint.switchId() = 10;
  endpoint.isAttached() = true;
  endpoint.portId() = 79;
  hwConnectivityMap.emplace(swPort->getID(), endpoint);

  auto dsfNode1 = makeDsfNode(10, "fdswB");
  auto dsfNode2 = makeDsfNode(20, "fdswA");

  auto dsfNodeMap = std::make_shared<MultiSwitchDsfNodeMap>();
  dsfNodeMap->addNode(dsfNode1, getScope(dsfNode1));
  dsfNodeMap->addNode(dsfNode2, getScope(dsfNode2));
  newState->resetDsfNodes(dsfNodeMap);

  // update
  StateDelta delta(oldState, newState);
  fabricConnectivityManager_->stateUpdated(delta);

  const auto expectedConnectivityMap =
      fabricConnectivityManager_->processConnectivityInfo(hwConnectivityMap);

  for (const auto& expectedConnectivity : expectedConnectivityMap) {
    const auto& neighbor = expectedConnectivity.second;
    EXPECT_EQ(expectedConnectivityMap.size(), 1);
    EXPECT_NE(*neighbor.expectedSwitchId(), *neighbor.switchId());
    EXPECT_EQ(neighbor.expectedPortName(), "fab1/2/3");
    EXPECT_NE(neighbor.expectedSwitchName(), neighbor.switchName());
    EXPECT_NE(neighbor.expectedPortName(), neighbor.portName());
    EXPECT_TRUE(neighbor.expectedPortId().has_value());
    // portId = 79 results no portname
    EXPECT_FALSE(neighbor.portName().has_value());
  }

  // there is info mismatch here
  EXPECT_TRUE(
      fabricConnectivityManager_->isConnectivityInfoMismatch(PortID(1)));
  EXPECT_TRUE(fabricConnectivityManager_->isConnectivityInfoMissing(PortID(1)));
}

TEST_F(FabricConnectivityManagerTest, nonFabricPorts) {
  auto oldState = std::make_shared<SwitchState>();
  auto newState = std::make_shared<SwitchState>();

  // create port with neighbor connectivity
  std::shared_ptr<Port> swPort = makePort(1, cfg::PortType::INTERFACE_PORT);
  swPort->setExpectedNeighborReachability(
      createPortNeighbor("fab1/2/3", "fdswA"));
  newState->getPorts()->addNode(swPort, getScope(swPort));

  // update
  StateDelta delta(oldState, newState);
  fabricConnectivityManager_->stateUpdated(delta);

  std::map<PortID, FabricEndpoint> hwConnectivityMap;
  // ensure that no connectivity info is created for non fabric port
  EXPECT_EQ(fabricConnectivityManager_->getConnectivityInfo().size(), 0);
}

// test case below mimics both invalid cfg scenario and cabling issues
TEST_F(FabricConnectivityManagerTest, validateMissingNeighborInfo) {
  auto oldState = std::make_shared<SwitchState>();
  auto newState = std::make_shared<SwitchState>();

  // create port with neighbor connectivity
  std::shared_ptr<Port> swPort = makePort(1);
  swPort->setExpectedNeighborReachability(
      createPortNeighbor("fab1/2/3", "fdswA"));
  newState->getPorts()->addNode(swPort, getScope(swPort));

  std::map<PortID, FabricEndpoint> hwConnectivityMap;
  FabricEndpoint endpoint;
  endpoint.switchId() = 10;
  endpoint.isAttached() = true;
  endpoint.portId() = 79;
  hwConnectivityMap.emplace(swPort->getID(), endpoint);

  auto dsfNode1 = makeDsfNode(10, "fdswB");
  // explicitly don't add dsfNode for fdswA to mimic missing info

  auto dsfNodeMap = std::make_shared<MultiSwitchDsfNodeMap>();
  dsfNodeMap->addNode(dsfNode1, getScope(dsfNode1));

  newState->resetDsfNodes(dsfNodeMap);

  // update
  StateDelta delta(oldState, newState);
  fabricConnectivityManager_->stateUpdated(delta);

  const auto expectedConnectivityMap =
      fabricConnectivityManager_->processConnectivityInfo(hwConnectivityMap);

  for (const auto& expectedConnectivity : expectedConnectivityMap) {
    const auto& neighbor = expectedConnectivity.second;
    EXPECT_EQ(expectedConnectivityMap.size(), 1);
    EXPECT_EQ(neighbor.expectedPortName(), "fab1/2/3");
    EXPECT_NE(neighbor.expectedSwitchName(), neighbor.switchName());
    EXPECT_NE(neighbor.expectedPortName(), neighbor.portName());
    // portId = 79 results no portname
    EXPECT_FALSE(neighbor.portName().has_value());
    // can't get it since dsf node is missing (fdswA)
    EXPECT_FALSE(neighbor.expectedPortId().has_value());
    EXPECT_FALSE(neighbor.expectedSwitchId().has_value());
  }

  // there is info mismatch here, since portName = fab1/2/4, but expected
  // fab1/2/3
  EXPECT_TRUE(
      fabricConnectivityManager_->isConnectivityInfoMismatch(PortID(1)));
  // there is info missing on expected switch id, portId (because of dsf node
  // missing)
  EXPECT_TRUE(fabricConnectivityManager_->isConnectivityInfoMissing(PortID(1)));
}

} // namespace facebook::fboss
