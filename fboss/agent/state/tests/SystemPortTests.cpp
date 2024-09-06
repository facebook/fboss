/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/state/SystemPort.h"
#include "fboss/agent/test/TestUtils.h"

#include <gtest/gtest.h>

using namespace facebook::fboss;

TEST(SystemPort, SerDeserSystemPort) {
  auto sysPort = makeSysPort("olympic");
  auto serialized = sysPort->toThrift();
  auto sysPortBack = std::make_shared<SystemPort>(serialized);

  EXPECT_TRUE(*sysPort == *sysPortBack);
}

TEST(SystemPort, SerDeserSystemPortNoQos) {
  auto sysPort = makeSysPort(std::nullopt);
  auto serialized = sysPort->toThrift();
  auto sysPortBack = std::make_shared<SystemPort>(serialized);
  EXPECT_TRUE(*sysPort == *sysPortBack);
}

TEST(SystemPort, SerDeserSwitchState) {
  auto state = std::make_shared<SwitchState>();

  auto sysPort1 = makeSysPort("olympic", 1);
  auto sysPort2 = makeSysPort("olympic", 2);

  HwSwitchMatcher scope{std::unordered_set<SwitchID>{SwitchID(0)}};
  state->getSystemPorts()->addNode(sysPort1, scope);
  state->getSystemPorts()->addNode(sysPort2, scope);

  auto serialized = state->toThrift();
  auto stateBack = SwitchState::fromThrift(serialized);

  // Check all systemPorts should be there
  for (auto sysPortID : {SystemPortID(1), SystemPortID(2)}) {
    EXPECT_TRUE(
        *state->getSystemPorts()->getNodeIf(sysPortID) ==
        *stateBack->getSystemPorts()->getNodeIf(sysPortID));
  }
}

TEST(SystemPort, AddRemove) {
  auto state = std::make_shared<SwitchState>();

  auto sysPort1 = makeSysPort("olympic", 1);
  auto sysPort2 = makeSysPort("olympic", 2);

  HwSwitchMatcher scope{std::unordered_set<SwitchID>{SwitchID(0)}};
  state->getSystemPorts()->addNode(sysPort1, scope);
  state->getSystemPorts()->addNode(sysPort2, scope);
  state->getSystemPorts()->removeNode(SystemPortID(1));
  EXPECT_EQ(state->getSystemPorts()->getNodeIf(SystemPortID(1)), nullptr);
  EXPECT_NE(state->getSystemPorts()->getNodeIf(SystemPortID(2)), nullptr);
}

TEST(SystemPort, GetScope) {
  auto state = std::make_shared<SwitchState>();

  auto sysPort1 = makeSysPort("olympic", 1);

  HwSwitchMatcher scope{std::unordered_set<SwitchID>{SwitchID(0)}};
  state->getSystemPorts()->addNode(sysPort1, scope);
  EXPECT_EQ(
      state->getSystemPorts()->getNodeAndScope(sysPort1->getID()).second,
      scope);
}

TEST(SystemPort, Modify) {
  {
    auto state = std::make_shared<SwitchState>();
    auto origSysPorts = state->getSystemPorts();
    EXPECT_EQ(origSysPorts.get(), origSysPorts->modify(&state));
    state->publish();
    EXPECT_NE(origSysPorts.get(), origSysPorts->modify(&state));
    EXPECT_NE(origSysPorts.get(), state->getSystemPorts().get());
  }
  {
    // Remote sys ports modify
    auto state = std::make_shared<SwitchState>();
    auto origRemoteSysPorts = state->getRemoteSystemPorts();
    EXPECT_EQ(origRemoteSysPorts.get(), origRemoteSysPorts->modify(&state));
    state->publish();
    EXPECT_NE(origRemoteSysPorts.get(), origRemoteSysPorts->modify(&state));
    EXPECT_NE(origRemoteSysPorts.get(), state->getRemoteSystemPorts().get());
  }
}

TEST(SystemPort, sysPortApplyConfig) {
  auto platform = createMockPlatform();
  auto stateV0 = std::make_shared<SwitchState>();
  addSwitchInfo(stateV0, cfg::SwitchType::VOQ, kVoqSwitchIdBegin /* switchId*/);
  auto config = testConfigA(cfg::SwitchType::VOQ);
  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  ASSERT_NE(nullptr, stateV1);
  EXPECT_EQ(
      stateV1->getSystemPorts()->numNodes(), stateV1->getPorts()->numNodes());
  // Flip one port to fabric port type and see that sys ports are updated
  config.ports()->begin()->portType() = cfg::PortType::FABRIC_PORT;
  // Prune the interface corresponding to now changed port type
  auto sysPortRange = stateV1->getAssociatedSystemPortRangeIf(
      PortID(*config.ports()->begin()->logicalID()));
  auto intfIDToPrune =
      *sysPortRange->minimum() + *config.ports()->begin()->logicalID();
  std::vector<cfg::Interface> intfs;
  for (const auto& intf : *config.interfaces()) {
    if (*intf.intfID() != intfIDToPrune) {
      intfs.push_back(intf);
    }
  }
  config.interfaces() = intfs;

  auto stateV2 = publishAndApplyConfig(stateV1, &config, platform.get());
  ASSERT_NE(nullptr, stateV2);
  EXPECT_EQ(
      stateV2->getSystemPorts()->numNodes(),
      stateV2->getPorts()->numNodes() - 1);
}

TEST(SystemPort, sysPortNameApplyConfig) {
  auto platform = createMockPlatform();
  auto stateV0 = std::make_shared<SwitchState>();
  addSwitchInfo(stateV0, cfg::SwitchType::VOQ, kVoqSwitchIdBegin /* switchId*/);
  auto config = testConfigA(cfg::SwitchType::VOQ);
  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  ASSERT_NE(nullptr, stateV1);
  EXPECT_EQ(
      stateV1->getSystemPorts()->numNodes(), stateV1->getPorts()->numNodes());
  auto nodeName =
      *config.dsfNodes()->find(SwitchID(kVoqSwitchIdBegin))->second.name();
  for (auto portMap : std::as_const(*stateV1->getPorts())) {
    for (auto port : std::as_const(*portMap.second)) {
      auto sysPortName =
          folly::sformat("{}:{}", nodeName, port.second->getName());
      XLOG(DBG2) << " Looking for sys port : " << sysPortName;
      EXPECT_NE(
          nullptr, stateV1->getSystemPorts()->getSystemPortIf(sysPortName));
    }
  }
}
TEST(SystemPort, GetLocalSwitchPortsBySwitchId) {
  auto platform = createMockPlatform();
  auto stateV0 = std::make_shared<SwitchState>();
  addSwitchInfo(stateV0, cfg::SwitchType::VOQ, kVoqSwitchIdBegin /* switchId*/);
  auto config = testConfigA(cfg::SwitchType::VOQ);
  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  ASSERT_NE(nullptr, stateV1);
  auto localSwitchId = kVoqSwitchIdBegin;
  auto mySysPorts = stateV1->getSystemPorts(SwitchID(localSwitchId));
  EXPECT_EQ(mySysPorts->size(), stateV1->getSystemPorts()->numNodes());
  // No remote sys ports
  EXPECT_EQ(stateV1->getSystemPorts(SwitchID(localSwitchId + 1))->size(), 0);
}

TEST(SystemPort, GetRemoteSwitchPortsBySwitchId) {
  auto platform = createMockPlatform();
  auto stateV0 = std::make_shared<SwitchState>();
  addSwitchInfo(stateV0, cfg::SwitchType::VOQ, kVoqSwitchIdBegin /* switchId*/);
  auto config = testConfigA(cfg::SwitchType::VOQ);
  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  int64_t remoteSwitchId = 100;
  HwSwitchMatcher scope{
      std::unordered_set<SwitchID>{SwitchID(kVoqSwitchIdBegin)}};
  auto sysPort1 = makeSysPort("olympic", 1, remoteSwitchId);
  auto sysPort2 = makeSysPort("olympic", 2, remoteSwitchId);
  auto stateV2 = stateV1->clone();
  auto remoteSysPorts = stateV2->getRemoteSystemPorts()->modify(&stateV2);
  remoteSysPorts->addNode(sysPort1, scope);
  remoteSysPorts->addNode(sysPort2, scope);
  EXPECT_EQ(stateV2->getSystemPorts(SwitchID(remoteSwitchId))->size(), 2);
}
