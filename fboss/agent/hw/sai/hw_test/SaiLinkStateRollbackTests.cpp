/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/hw_test/SaiRollbackTest.h"

#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/SwitchState.h"

namespace facebook::fboss {

TEST_F(SaiRollbackTest, rollbackLinkUpAndDown) {
  auto verify = [this] {
    auto origState = getProgrammedState();
    bringDownPort(masterLogicalPortIds()[0]);
    auto link1DownState = getProgrammedState();
    // Rollback link down
    rollback(StateDelta(origState, getProgrammedState()));
    // Rollback link up
    rollback(StateDelta(link1DownState, getProgrammedState()));
    // Back to square 1
    rollback(StateDelta(origState, getProgrammedState()));
    for (auto portID : masterLogicalInterfacePortIds()) {
      const auto& port = getProgrammedState()->getPorts()->getNodeIf(portID);
      EXPECT_TRUE(port->isPortUp());
    }
  };
  verifyAcrossWarmBoots([]() {}, verify);
}
} // namespace facebook::fboss
