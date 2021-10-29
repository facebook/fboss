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
#include "fboss/agent/test/TestUtils.h"

using namespace facebook::fboss;

TEST(ThriftySwitchState, BasicTest) {
  auto state = SwitchState();
  auto stateBack = SwitchState::fromThrift(state.toThrift());
  EXPECT_EQ(state, *stateBack);
}

TEST(ThriftySwitchState, PortMap) {
  auto port1 = std::make_shared<Port>(PortID(1), "eth2/1/1");
  auto port2 = std::make_shared<Port>(PortID(2), "eth2/2/1");
  auto portMap = std::make_shared<PortMap>();
  portMap->addPort(port1);
  portMap->addPort(port2);

  auto state = SwitchState();
  state.resetPorts(portMap);

  validateThriftyMigration(*portMap);

  auto stateBack = SwitchState::fromThrift(state.toThrift());
  EXPECT_EQ(state, *stateBack);
}
