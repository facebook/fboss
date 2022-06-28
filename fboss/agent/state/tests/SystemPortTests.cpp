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

std::unique_ptr<SystemPort> makeSysPort(
    const std::optional<std::string>& qosPolicy) {
  auto sysPort = std::make_unique<SystemPort>(SystemPortID(1));
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
