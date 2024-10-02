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
      cfg::AsicType asicType = cfg::AsicType::ASIC_TYPE_RAMON3,
      PlatformType platformType = PlatformType::PLATFORM_MERU800BFA) {
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
  std::optional<FabricEndpoint> getCurrentConnectivity(PortID port) const {
    std::optional<FabricEndpoint> portConnectivity;
    auto curConnectivity = fabricConnectivityManager_->getConnectivityInfo();
    auto itr = curConnectivity.find(port);
    if (itr != curConnectivity.end()) {
      portConnectivity = itr->second;
    }
    return portConnectivity;
  }

  std::map<PortID, FabricEndpoint> processConnectivityInfo(
      const std::map<PortID, FabricEndpoint>& hwConnectivity) {
    for (const auto& [port, endpoint] : hwConnectivity) {
      auto beforeConnectivity = getCurrentConnectivity(port);
      auto delta = fabricConnectivityManager_->processConnectivityInfoForPort(
          port, endpoint);
      auto afterConectivity = getCurrentConnectivity(port);
      if (beforeConnectivity == afterConectivity) {
        EXPECT_EQ(delta, std::nullopt);
      } else {
        multiswitch::FabricConnectivityDelta expected;
        if (beforeConnectivity.has_value()) {
          expected.oldConnectivity() = *beforeConnectivity;
        }
        if (afterConectivity.has_value()) {
          expected.newConnectivity() = *afterConectivity;
        }
        EXPECT_EQ(*delta, expected);
      }
    }
    return fabricConnectivityManager_->getConnectivityInfo();
  }
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
      9; // known from platforom mapping for Meru800biaPlatformMapping
  endpoint.switchId() = 10;
  endpoint.isAttached() = true;
  hwConnectivityMap.emplace(swPort->getID(), endpoint);

  auto dsfNode = makeDsfNode(
      10,
      "rdswA",
      cfg::AsicType::ASIC_TYPE_JERICHO3,
      PlatformType::PLATFORM_MERU800BIA);
  auto dsfNodeMap = std::make_shared<MultiSwitchDsfNodeMap>();
  dsfNodeMap->addNode(dsfNode, getScope(dsfNode));
  newState->resetDsfNodes(dsfNodeMap);

  StateDelta delta(oldState, newState);
  fabricConnectivityManager_->stateUpdated(delta);

  const auto expectedConnectivityMap =
      processConnectivityInfo(hwConnectivityMap);
  EXPECT_EQ(expectedConnectivityMap.size(), 1);

  for (const auto& expectedConnectivity : expectedConnectivityMap) {
    const auto& neighbor = expectedConnectivity.second;
    // remote offset for jericho3 is 1024
    EXPECT_EQ(neighbor.expectedPortId(), 9);
    EXPECT_EQ(neighbor.expectedPortName(), "fab1/9/2");
    EXPECT_EQ(*neighbor.portId(), 9);
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
  endpoint.portId() = 12; // known from platforom mapping for ramon3
  endpoint.switchId() = 10;
  endpoint.isAttached() = true;

  hwConnectivityMap.emplace(swPort->getID(), endpoint);

  auto dsfNode = makeDsfNode(10, "fdswA", cfg::AsicType::ASIC_TYPE_RAMON3);
  auto dsfNodeMap = std::make_shared<MultiSwitchDsfNodeMap>();
  dsfNodeMap->addNode(dsfNode, getScope(dsfNode));
  newState->resetDsfNodes(dsfNodeMap);

  StateDelta delta(oldState, newState);
  fabricConnectivityManager_->stateUpdated(delta);

  auto expectedConnectivityMap = processConnectivityInfo(hwConnectivityMap);
  EXPECT_EQ(expectedConnectivityMap.size(), 1);

  for (const auto& expectedConnectivity : expectedConnectivityMap) {
    const auto& neighbor = expectedConnectivity.second;
    EXPECT_EQ(neighbor.expectedPortId(), 12);
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
  // Update port to have no expected neighbors
  newState->publish();
  auto newerState = newState->clone();
  auto newPort = swPort->modify(&newerState);
  newPort->setExpectedNeighborReachability({});
  fabricConnectivityManager_->stateUpdated(StateDelta(newState, newerState));

  expectedConnectivityMap = processConnectivityInfo(hwConnectivityMap);
  EXPECT_EQ(expectedConnectivityMap.size(), 1);

  for (const auto& expectedConnectivity : expectedConnectivityMap) {
    const auto& neighbor = expectedConnectivity.second;
    EXPECT_FALSE(neighbor.expectedPortId().has_value());
    EXPECT_FALSE(neighbor.expectedSwitchId().has_value());
    EXPECT_EQ(neighbor.switchId(), 10);
    EXPECT_FALSE(neighbor.expectedSwitchName().has_value());
    EXPECT_FALSE(neighbor.expectedPortName().has_value());
    EXPECT_TRUE(*neighbor.isAttached());
  }

  EXPECT_TRUE(fabricConnectivityManager_->isConnectivityInfoMissing(PortID(1)));
  EXPECT_TRUE(
      fabricConnectivityManager_->isConnectivityInfoMismatch(PortID(1)));
  // Update port to have expected neighbors again
  fabricConnectivityManager_->stateUpdated(StateDelta(newerState, newState));
  expectedConnectivityMap = processConnectivityInfo(hwConnectivityMap);
  EXPECT_EQ(expectedConnectivityMap.size(), 1);

  for (const auto& expectedConnectivity : expectedConnectivityMap) {
    const auto& neighbor = expectedConnectivity.second;
    EXPECT_EQ(neighbor.expectedPortId(), 12);
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

TEST_F(FabricConnectivityManagerTest, validateNoExpectedConnectivity) {
  auto oldState = std::make_shared<SwitchState>();
  auto newState = std::make_shared<SwitchState>();

  // create port with neighbor connectivity
  std::shared_ptr<Port> swPort = makePort(1);
  swPort->setExpectedNeighborReachability({});
  newState->getPorts()->addNode(swPort, getScope(swPort));

  std::map<PortID, FabricEndpoint> hwConnectivityMap;
  FabricEndpoint endpoint;
  endpoint.portId() = 12; // known from platforom mapping for ramon3
  endpoint.switchId() = 10;
  endpoint.isAttached() = true;

  hwConnectivityMap.emplace(swPort->getID(), endpoint);

  auto dsfNode = makeDsfNode(10, "fdswA", cfg::AsicType::ASIC_TYPE_RAMON3);
  auto dsfNodeMap = std::make_shared<MultiSwitchDsfNodeMap>();
  dsfNodeMap->addNode(dsfNode, getScope(dsfNode));
  newState->resetDsfNodes(dsfNodeMap);

  StateDelta delta(oldState, newState);
  fabricConnectivityManager_->stateUpdated(delta);

  const auto expectedConnectivityMap =
      processConnectivityInfo(hwConnectivityMap);
  EXPECT_EQ(expectedConnectivityMap.size(), 1);

  for (const auto& expectedConnectivity : expectedConnectivityMap) {
    const auto& neighbor = expectedConnectivity.second;
    EXPECT_FALSE(neighbor.expectedPortId().has_value());
    EXPECT_FALSE(neighbor.expectedSwitchId().has_value());
    EXPECT_EQ(neighbor.switchId(), 10);
    EXPECT_FALSE(neighbor.expectedSwitchName().has_value());
    EXPECT_FALSE(neighbor.expectedPortName().has_value());
    EXPECT_TRUE(*neighbor.isAttached());
  }

  EXPECT_TRUE(fabricConnectivityManager_->isConnectivityInfoMissing(PortID(1)));
  EXPECT_TRUE(
      fabricConnectivityManager_->isConnectivityInfoMismatch(PortID(1)));
}

TEST_F(FabricConnectivityManagerTest, connectivityRetainedOnPortUpdates) {
  auto oldState = std::make_shared<SwitchState>();
  auto newState = std::make_shared<SwitchState>();

  // create port with neighbor connectivity
  std::shared_ptr<Port> swPort = makePort(1);
  swPort->setExpectedNeighborReachability(
      createPortNeighbor("fab1/2/4", "fdswA"));
  newState->getPorts()->addNode(swPort, getScope(swPort));

  std::map<PortID, FabricEndpoint> hwConnectivityMap;
  FabricEndpoint endpoint;
  endpoint.portId() = 12; // known from platforom mapping for ramon3
  endpoint.switchId() = 10;
  endpoint.isAttached() = true;

  hwConnectivityMap.emplace(swPort->getID(), endpoint);

  auto dsfNode = makeDsfNode(10, "fdswA", cfg::AsicType::ASIC_TYPE_RAMON3);
  auto dsfNodeMap = std::make_shared<MultiSwitchDsfNodeMap>();
  dsfNodeMap->addNode(dsfNode, getScope(dsfNode));
  newState->resetDsfNodes(dsfNodeMap);

  StateDelta delta(oldState, newState);
  fabricConnectivityManager_->stateUpdated(delta);

  processConnectivityInfo(hwConnectivityMap);

  auto prevConnectivityInfo = getCurrentConnectivity(swPort->getID());
  auto assertConnectivity = [](const FabricEndpoint& endpoint) {
    EXPECT_EQ(endpoint.expectedPortId(), 12);
    EXPECT_EQ(endpoint.expectedSwitchId(), 10);
    EXPECT_EQ(endpoint.switchId(), 10);
    EXPECT_EQ(endpoint.expectedSwitchName(), "fdswA");
    EXPECT_EQ(endpoint.expectedPortName(), "fab1/2/4");
    EXPECT_TRUE(*endpoint.isAttached());
    EXPECT_EQ(*endpoint.expectedPortId(), *endpoint.portId());
    EXPECT_EQ(endpoint.expectedPortName(), endpoint.portName());
  };

  EXPECT_TRUE(prevConnectivityInfo.has_value());
  assertConnectivity(*prevConnectivityInfo);
  auto newerState = newState->clone();
  auto curPort = newerState->getPorts()->getPort(swPort->getName());
  auto newPort = curPort->modify(&newerState);
  EXPECT_EQ(newPort->getAdminState(), cfg::PortState::ENABLED);
  newPort->setAdminState(cfg::PortState::DISABLED);
  fabricConnectivityManager_->stateUpdated(StateDelta(newState, newerState));
  auto newConnectivityInfo = getCurrentConnectivity(swPort->getID());
  EXPECT_TRUE(newConnectivityInfo.has_value());
  assertConnectivity(*newConnectivityInfo);
  EXPECT_EQ(prevConnectivityInfo, newConnectivityInfo);
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
      processConnectivityInfo(hwConnectivityMap);
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

  // unattached port implies connectivity missing
  EXPECT_TRUE(fabricConnectivityManager_->isConnectivityInfoMissing(PortID(1)));
  // unattached port does not imply connectivity mismatch
  EXPECT_FALSE(
      fabricConnectivityManager_->isConnectivityInfoMismatch(PortID(1)));

  // another case for unattached endpoint is where its not having
  // neighbor connectivity cfg as well ..this is case of down fabric ports
  // and should be discounted
  auto newState2 = std::make_shared<SwitchState>();
  std::shared_ptr<Port> swPort2 = makePort(2);
  newState->getPorts()->addNode(swPort2, getScope(swPort2));

  // update
  StateDelta delta2(newState, newState2);
  fabricConnectivityManager_->stateUpdated(delta2);

  // No connectivity expected nor available - neither mismatch nor missing
  EXPECT_FALSE(
      fabricConnectivityManager_->isConnectivityInfoMismatch(PortID(2)));
  EXPECT_FALSE(
      fabricConnectivityManager_->isConnectivityInfoMissing(PortID(2)));
  // Non existent port - neither mismatch nor missing
  EXPECT_FALSE(
      fabricConnectivityManager_->isConnectivityInfoMismatch(PortID(42)));
  EXPECT_FALSE(
      fabricConnectivityManager_->isConnectivityInfoMissing(PortID(42)));
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
      processConnectivityInfo(hwConnectivityMap);

  for (const auto& expectedConnectivity : expectedConnectivityMap) {
    const auto& neighbor = expectedConnectivity.second;
    EXPECT_EQ(expectedConnectivityMap.size(), 1);
    EXPECT_NE(*neighbor.expectedSwitchId(), *neighbor.switchId());
    EXPECT_EQ(neighbor.expectedPortName(), "fab1/2/3");
    EXPECT_NE(neighbor.expectedSwitchName(), neighbor.switchName());
    EXPECT_NE(neighbor.expectedPortName(), neighbor.portName());
    EXPECT_TRUE(neighbor.expectedPortId().has_value());

    // PortID 79 corresponds to fab1/40/2 in Meru800bfa platform map
    EXPECT_EQ(neighbor.portName(), "fab1/40/2");
  }

  // there is info mismatch here
  EXPECT_TRUE(
      fabricConnectivityManager_->isConnectivityInfoMismatch(PortID(1)));

  // but connectivity info is present
  EXPECT_FALSE(
      fabricConnectivityManager_->isConnectivityInfoMissing(PortID(1)));
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
      processConnectivityInfo(hwConnectivityMap);

  for (const auto& expectedConnectivity : expectedConnectivityMap) {
    const auto& neighbor = expectedConnectivity.second;
    EXPECT_EQ(expectedConnectivityMap.size(), 1);
    EXPECT_EQ(neighbor.expectedPortName(), "fab1/2/3");
    EXPECT_NE(neighbor.expectedSwitchName(), neighbor.switchName());
    EXPECT_NE(neighbor.expectedPortName(), neighbor.portName());

    // PortID 79 corresponds to fab1/40/2 in Meru800bfa platform map
    EXPECT_EQ(neighbor.portName(), "fab1/40/2");

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

TEST_F(FabricConnectivityManagerTest, validateConnectivityDelta) {
  auto oldState = std::make_shared<SwitchState>();
  auto newState = std::make_shared<SwitchState>();
  constexpr auto kRemotePortId = 12;
  constexpr auto kLocalPortId = 1;

  // create port with neighbor connectivity
  std::shared_ptr<Port> swPort = makePort(kLocalPortId);
  swPort->setExpectedNeighborReachability(
      createPortNeighbor("fab1/2/4", "fdswA"));
  newState->getPorts()->addNode(swPort, getScope(swPort));

  auto dsfNode10 = makeDsfNode(10, "fdswA", cfg::AsicType::ASIC_TYPE_RAMON3);
  auto dsfNode11 = makeDsfNode(11, "fdswB", cfg::AsicType::ASIC_TYPE_RAMON3);
  auto dsfNodeMap = std::make_shared<MultiSwitchDsfNodeMap>();
  dsfNodeMap->addNode(dsfNode10, getScope(dsfNode10));
  dsfNodeMap->addNode(dsfNode11, getScope(dsfNode11));
  newState->resetDsfNodes(dsfNodeMap);
  fabricConnectivityManager_->stateUpdated(StateDelta(oldState, newState));

  FabricEndpoint endpoint;
  endpoint.portId() = kRemotePortId; // known from platform mapping for ramon3
  endpoint.switchId() = 10;
  endpoint.isAttached() = true;
  // Update connectivity before processing port. Old connectivity should
  // be null and new connectivity should get updated
  auto delta1 = fabricConnectivityManager_->processConnectivityInfoForPort(
      PortID(kLocalPortId), endpoint);
  EXPECT_TRUE(delta1.has_value());
  EXPECT_TRUE(delta1->oldConnectivity().has_value());
  EXPECT_TRUE(delta1->newConnectivity().has_value());

  auto delta2 = fabricConnectivityManager_->processConnectivityInfoForPort(
      PortID(kLocalPortId), endpoint);
  // Same connectivity info, should not expect any changes.
  EXPECT_FALSE(delta2.has_value());
  // Update switch id
  endpoint.switchId() = 11;
  auto delta3 = fabricConnectivityManager_->processConnectivityInfoForPort(
      PortID(kLocalPortId), endpoint);

  EXPECT_TRUE(delta3.has_value());
  EXPECT_TRUE(delta3->oldConnectivity().has_value());
  EXPECT_TRUE(delta3->newConnectivity().has_value());
  EXPECT_EQ(*delta3->oldConnectivity(), *delta1->newConnectivity());
  // Same connectivity info, should not expect any changes.
  auto delta4 = fabricConnectivityManager_->processConnectivityInfoForPort(
      PortID(kLocalPortId), endpoint);
  EXPECT_FALSE(delta4.has_value());
}

TEST_F(FabricConnectivityManagerTest, virtualDeviceToRemoteConnectionGroups) {
  auto oldState = std::make_shared<SwitchState>();
  auto newState = std::make_shared<SwitchState>();
  constexpr auto kRemotePortIdBase = 79;
  constexpr auto kLocalPortIdBase = 1;
  constexpr auto kRemoteSwitchIdBase = 10;

  // create port with neighbor connectivity
  for (auto i = 1; i <= 3; ++i) {
    std::shared_ptr<Port> swPort = makePort(i);
    newState->getPorts()->addNode(swPort, getScope(swPort));
  }

  auto dsfNode10 = makeDsfNode(
      kRemoteSwitchIdBase, "fdswA", cfg::AsicType::ASIC_TYPE_RAMON3);
  auto dsfNode11 = makeDsfNode(
      kRemoteSwitchIdBase + 1, "fdswB", cfg::AsicType::ASIC_TYPE_RAMON3);
  auto dsfNodeMap = std::make_shared<MultiSwitchDsfNodeMap>();
  dsfNodeMap->addNode(dsfNode10, getScope(dsfNode10));
  dsfNodeMap->addNode(dsfNode11, getScope(dsfNode11));
  newState->resetDsfNodes(dsfNodeMap);
  fabricConnectivityManager_->stateUpdated(StateDelta(oldState, newState));

  // 1 port each from each virtual device 2 a remote endpoint
  for (auto i = 0; i < 2; ++i) {
    FabricEndpoint endpoint;
    endpoint.portId() = kRemotePortIdBase + i;
    endpoint.switchId() = kRemoteSwitchIdBase + i;
    endpoint.isAttached() = true;
    fabricConnectivityManager_->processConnectivityInfoForPort(
        PortID(kLocalPortIdBase + i), endpoint);
  }
  auto getVirtualDevice = [](PortID port) { return port == PortID(1) ? 0 : 1; };
  {
    /*
     * VD 0 -> {Port1, switchID 10}
     * VD 1 -> {Port2, switchID 11}
     */
    auto remoteConnectionGroups =
        fabricConnectivityManager_->getVirtualDeviceToRemoteConnectionGroups(
            getVirtualDevice);
    // Symmetric connectivity
    EXPECT_EQ(
        FabricConnectivityManager::virtualDevicesWithAsymmetricConnectivity(
            remoteConnectionGroups),
        0);
    EXPECT_EQ(remoteConnectionGroups.size(), 2);
    const auto& connectionGroupDevice0 = remoteConnectionGroups.find(0)->second;
    EXPECT_EQ(connectionGroupDevice0.size(), 1);
    auto numConnectionsDevice0 = connectionGroupDevice0.begin()->first;
    EXPECT_EQ(numConnectionsDevice0, 1);
    auto remoteEndpointDevice0 =
        *connectionGroupDevice0.begin()->second.begin();
    EXPECT_EQ(
        remoteEndpointDevice0.connectingPorts(),
        std::vector<std::string>({"port1"}));
    const auto& connectionGroupDevice1 = remoteConnectionGroups.find(1)->second;
    auto remoteEndpointDevice1 =
        *connectionGroupDevice1.begin()->second.begin();
    EXPECT_EQ(
        remoteEndpointDevice1.connectingPorts(),
        std::vector<std::string>({"port2"}));
  }
  // Add endpoint reachability of port 3
  FabricEndpoint endpoint;
  endpoint.portId() = kRemotePortIdBase + 1;
  endpoint.switchId() = kRemoteSwitchIdBase + 1;
  endpoint.isAttached() = true;
  fabricConnectivityManager_->processConnectivityInfoForPort(
      PortID(3), endpoint);
  {
    /*
     * VD 0 -> {(Port 1, switchID 10)}
     * VD 1 -> {(Port2, switchID 11), (Port 3, switchID 11)}
     */
    auto remoteConnectionGroups =
        fabricConnectivityManager_->getVirtualDeviceToRemoteConnectionGroups(
            getVirtualDevice);
    // Symmetric connectivity, VD0 has connectivity group of size 1 and
    // VD1 has connectivity group of size 2. But each VD is still symmetric
    EXPECT_EQ(
        FabricConnectivityManager::virtualDevicesWithAsymmetricConnectivity(
            remoteConnectionGroups),
        0);
    EXPECT_EQ(remoteConnectionGroups.size(), 2);
    const auto& connectionGroupDevice0 = remoteConnectionGroups.find(0)->second;
    EXPECT_EQ(connectionGroupDevice0.size(), 1);
    const auto& connectionGroupDevice1 = remoteConnectionGroups.find(1)->second;
    EXPECT_EQ(connectionGroupDevice1.size(), 1);
    // 1 connection from virtual device 0 to remote switchId10
    auto numConnectionsDevice0 = connectionGroupDevice0.begin()->first;
    EXPECT_EQ(numConnectionsDevice0, 1);
    auto remoteEndpointDevice0 =
        *connectionGroupDevice0.begin()->second.begin();
    EXPECT_EQ(
        remoteEndpointDevice0.connectingPorts(),
        std::vector<std::string>({"port1"}));
    // 2 connection from virtual device 1 to remote switchId11
    auto numConnectionsDevice1 = connectionGroupDevice1.begin()->first;
    EXPECT_EQ(numConnectionsDevice1, 2);
    auto remoteEndpointDevice1 =
        *connectionGroupDevice1.begin()->second.begin();
    EXPECT_EQ(
        remoteEndpointDevice1.connectingPorts(),
        std::vector<std::string>({"port2", "port3"}));
  }
  // Now make all connections map to virtual device 0
  {
    /*
     * VD 0 -> {
     * 3->(Port 1, switchID 10), (Port2, switchID 11), (Port 3, switchID 11)}
     */
    auto getVirtualDevice0 = [](PortID /*port*/) { return 0; };
    auto remoteConnectionGroups =
        fabricConnectivityManager_->getVirtualDeviceToRemoteConnectionGroups(
            getVirtualDevice0);
    // Asymmetric connectivity, VD0 has connectivity groups of size 1 (to switch
    // 10) and 2 (to switch11)
    EXPECT_EQ(
        FabricConnectivityManager::virtualDevicesWithAsymmetricConnectivity(
            remoteConnectionGroups),
        1);
    EXPECT_EQ(remoteConnectionGroups.size(), 1);
    // No connectivity from VD 1
    EXPECT_EQ(remoteConnectionGroups.find(1), remoteConnectionGroups.end());
    const auto& connectionGroupDevice0 = remoteConnectionGroups.find(0)->second;
    // Asymmetric connectivity
    EXPECT_EQ(connectionGroupDevice0.size(), 2);
    auto connectivityGroupSize1 = connectionGroupDevice0.find(1)->second;
    // Num remote endpoint == 1
    EXPECT_EQ(connectivityGroupSize1.size(), 1);
    auto remoteEndpointGroup1 = *connectivityGroupSize1.begin();
    EXPECT_EQ(*remoteEndpointGroup1.switchId(), kRemoteSwitchIdBase);
    EXPECT_EQ(
        remoteEndpointGroup1.connectingPorts(),
        std::vector<std::string>({"port1"}));
    // Num remote endpoint == 2
    auto connectivityGroupSize2 = connectionGroupDevice0.find(2)->second;
    EXPECT_EQ(connectivityGroupSize2.size(), 1);
    auto remoteEndpointGroup2 = *connectivityGroupSize2.begin();
    EXPECT_EQ(*remoteEndpointGroup2.switchId(), kRemoteSwitchIdBase + 1);
    EXPECT_EQ(
        remoteEndpointGroup2.connectingPorts(),
        std::vector<std::string>({"port2", "port3"}));
  }
}
} // namespace facebook::fboss
