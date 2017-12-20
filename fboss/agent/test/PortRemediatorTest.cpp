/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/PortRemediator.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/hw/mock/MockPlatform.h"
#include "fboss/agent/hw/mock/MockPlatformPort.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/TestUtils.h"

#include <gtest/gtest.h>

namespace facebook { namespace fboss {
using namespace ::testing;

class PortRemediatorTest : public ::testing::Test {
 public:
  void setupPorts(std::vector<PortID> disabled, std::vector<PortID> down) {
    auto state = testStateA();
    auto ports = state->getPorts();
    // In testStateA(), ports are down but enabled. Default to up and enabled
    // so we can modify just those we want to have interesting behavior
    for (auto port : *ports) {
      port->setAdminState(cfg::PortState::ENABLED);
      port->setOperState(true);
    };
    for (const auto& id : disabled) {
      auto port = ports->getPort(id);
      port->setAdminState(cfg::PortState::DISABLED);
    }
    for (const auto& id : down) {
      auto port = ports->getPort(id);
      port->setOperState(false);
    }
    handle = createTestHandle(state, folly::none);
    portRemediator = std::make_unique<PortRemediator>(handle->getSw());
  }
  std::unique_ptr<HwTestHandle> handle;
  std::unique_ptr<PortRemediator> portRemediator;
};

TEST_F(PortRemediatorTest, AllEnabledAndUp) {
  setupPorts({}, {});
  auto unexpected = portRemediator->getUnexpectedDownPorts();
  EXPECT_EQ(0, unexpected.size());
}

TEST_F(PortRemediatorTest, OneEnabledAndDown) {
  setupPorts({}, {PortID(10)});
  auto unexpected = portRemediator->getUnexpectedDownPorts();
  EXPECT_EQ(1, unexpected.size());
}

TEST_F(PortRemediatorTest, OneDisabledAndDown) {
  setupPorts({PortID(10)}, {PortID(10)});
  auto unexpected = portRemediator->getUnexpectedDownPorts();
  EXPECT_EQ(0, unexpected.size());
}

}} // facebook::fboss
