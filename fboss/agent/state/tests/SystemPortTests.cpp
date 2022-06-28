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

std::shared_ptr<SystemPort> makeSysPort(
    const std::optional<std::string>& qosPolicy,
    int64_t sysPortId = 1) {
  auto sysPort = std::make_shared<SystemPort>(SystemPortID(sysPortId));
  sysPort->setSwitchId(SwitchID(1));
  sysPort->setPortName("sysPort1");
  sysPort->setCoreIndex(42);
  sysPort->setCorePortIndex(24);
  sysPort->setSpeedMbps(10000);
  sysPort->setNumVoqs(8);
  sysPort->setEnabled(true);
  sysPort->setQosPolicy(qosPolicy);
  return sysPort;
}

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

  auto serialized = state->toFollyDynamic();
  auto stateBack = SwitchState::fromFollyDynamic(serialized);

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
  auto state = std::make_shared<SwitchState>();
  auto origSysPorts = state->getSystemPorts();
  EXPECT_EQ(origSysPorts.get(), origSysPorts->modify(&state));
  state->publish();
  EXPECT_NE(origSysPorts.get(), origSysPorts->modify(&state));
}
