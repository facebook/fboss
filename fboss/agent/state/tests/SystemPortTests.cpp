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
  auto serialized = sysPort->toFollyDynamic();
  auto sysPortBack = SystemPort::fromFollyDynamic(serialized);

  EXPECT_TRUE(*sysPort == *sysPortBack);
}

TEST(SystemPort, SerDeserSystemPortNoQos) {
  auto sysPort = makeSysPort(std::nullopt);
  auto serialized = sysPort->toFollyDynamic();
  auto sysPortBack = SystemPort::fromFollyDynamic(serialized);
  EXPECT_TRUE(*sysPort == *sysPortBack);
}

TEST(SystemPort, SerDeserSwitchState) {
  auto state = std::make_shared<SwitchState>();

  auto sysPort1 = makeSysPort("olympic", 1);
  auto sysPort2 = makeSysPort("olympic", 2);

  state->addSystemPort(sysPort1);
  state->addSystemPort(sysPort2);

  auto serialized = state->toThrift();
  auto stateBack = SwitchState::fromThrift(serialized);

  // Check all systemPorts should be there
  for (auto sysPortID : {SystemPortID(1), SystemPortID(2)}) {
    EXPECT_TRUE(
        *state->getSystemPorts()->getSystemPort(sysPortID) ==
        *stateBack->getSystemPorts()->getSystemPort(sysPortID));
  }
}

TEST(SystemPort, AddRemove) {
  auto state = std::make_shared<SwitchState>();

  auto sysPort1 = makeSysPort("olympic", 1);
  auto sysPort2 = makeSysPort("olympic", 2);

  state->addSystemPort(sysPort1);
  state->addSystemPort(sysPort2);
  state->getSystemPorts()->removeSystemPort(SystemPortID(1));
  EXPECT_EQ(state->getSystemPorts()->getSystemPortIf(SystemPortID(1)), nullptr);
  EXPECT_NE(state->getSystemPorts()->getSystemPortIf(SystemPortID(2)), nullptr);
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
  auto config = testConfigA(cfg::SwitchType::VOQ);
  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  ASSERT_NE(nullptr, stateV1);
  EXPECT_EQ(stateV1->getSystemPorts()->size(), stateV1->getPorts()->size());
  // Flip one port to fabric port type and see that sys ports are updated
  config.ports()->begin()->portType() = cfg::PortType::FABRIC_PORT;
  auto stateV2 = publishAndApplyConfig(stateV1, &config, platform.get());
  ASSERT_NE(nullptr, stateV2);
  EXPECT_EQ(stateV2->getSystemPorts()->size(), stateV2->getPorts()->size() - 1);
}

TEST(SystemPort, sysPortApplyConfigSwitchTypeChange) {
  auto platform = createMockPlatform();
  auto stateV0 = std::make_shared<SwitchState>();
  auto config = testConfigA(cfg::SwitchType::VOQ);
  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  ASSERT_NE(nullptr, stateV1);
  EXPECT_EQ(stateV1->getSystemPorts()->size(), stateV1->getPorts()->size());
  config.switchSettings()->switchType() = cfg::SwitchType::NPU;
  config.switchSettings()->switchId().reset();
  auto stateV2 = publishAndApplyConfig(stateV1, &config, platform.get());
  ASSERT_NE(nullptr, stateV2);
  EXPECT_EQ(stateV2->getSystemPorts()->size(), 0);
}

TEST(SystemPort, sysPortApplyConfigSwitchIdChange) {
  auto platform = createMockPlatform();
  auto stateV0 = std::make_shared<SwitchState>();
  auto config = testConfigA(cfg::SwitchType::VOQ);
  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  ASSERT_NE(nullptr, stateV1);
  EXPECT_EQ(stateV1->getSystemPorts()->size(), stateV1->getPorts()->size());
  auto prevSwitchId = *config.switchSettings()->switchId();
  config = updateSwitchID(config, 2);

  auto stateV2 = publishAndApplyConfig(stateV1, &config, platform.get());
  ASSERT_NE(nullptr, stateV2);
  EXPECT_EQ(
      stateV2->getSystemPorts()->size(), stateV1->getSystemPorts()->size());

  for (const auto& idAndSysPort : std::as_const(*stateV2->getSystemPorts())) {
    const auto& sysPort = idAndSysPort.second;
    EXPECT_EQ(sysPort->getSwitchId(), SwitchID(2));
  }
}

TEST(SystemPort, sysPortNameApplyConfig) {
  auto platform = createMockPlatform();
  auto stateV0 = std::make_shared<SwitchState>();
  auto config = testConfigA(cfg::SwitchType::VOQ);
  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  ASSERT_NE(nullptr, stateV1);
  EXPECT_EQ(stateV1->getSystemPorts()->size(), stateV1->getPorts()->size());
  auto switchIdOpt = stateV1->getSwitchSettings()->getSwitchId();
  CHECK(switchIdOpt.has_value());
  auto switchId = *switchIdOpt;
  auto nodeName = *config.dsfNodes()->find(switchId)->second.name();
  for (auto port : std::as_const(*stateV1->getPorts())) {
    auto sysPortName =
        folly::sformat("{}:{}", nodeName, port.second->getName());
    XLOG(DBG2) << " Looking for sys port : " << sysPortName;
    EXPECT_NE(nullptr, stateV1->getSystemPorts()->getSystemPortIf(sysPortName));
  }
}
TEST(SystemPort, GetLocalSwitchPortsBySwitchId) {
  auto platform = createMockPlatform();
  auto stateV0 = std::make_shared<SwitchState>();
  auto config = testConfigA(cfg::SwitchType::VOQ);
  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  ASSERT_NE(nullptr, stateV1);
  auto mySwitchId = stateV1->getSwitchSettings()->getSwitchId();
  CHECK(mySwitchId) << "Switch ID must be set for VOQ switch";
  auto mySysPorts = stateV1->getSystemPorts(SwitchID(*mySwitchId));
  EXPECT_EQ(mySysPorts->size(), stateV1->getSystemPorts()->size());
  // No remote sys ports
  EXPECT_EQ(stateV1->getSystemPorts(SwitchID(*mySwitchId + 1))->size(), 0);
}

TEST(SystemPort, GetRemoteSwitchPortsBySwitchId) {
  auto platform = createMockPlatform();
  auto stateV0 = std::make_shared<SwitchState>();
  auto config = testConfigA(cfg::SwitchType::VOQ);
  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  int64_t remoteSwitchId = 100;
  auto sysPort1 = makeSysPort("olympic", 1, remoteSwitchId);
  auto sysPort2 = makeSysPort("olympic", 2, remoteSwitchId);
  auto stateV2 = stateV1->clone();
  auto remoteSysPorts = stateV2->getRemoteSystemPorts()->modify(&stateV2);
  remoteSysPorts->addSystemPort(sysPort1);
  remoteSysPorts->addSystemPort(sysPort2);
  EXPECT_EQ(stateV2->getSystemPorts(SwitchID(remoteSwitchId))->size(), 2);
}
