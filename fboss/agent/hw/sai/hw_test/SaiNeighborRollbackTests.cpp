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

#include "fboss/agent/hw/test/HwTestEcmpUtils.h"
#include "fboss/agent/hw/test/HwTestRouteUtils.h"
#include "fboss/agent/state/SwitchState.h"

namespace facebook::fboss {

TEST_F(SaiRollbackTest, rollbackNeighborResolution) {
  auto verify = [this] {
    auto origState = getProgrammedState();
    utility::EcmpSetupAnyNPorts4 v4EcmpHelper(
        getProgrammedState(), RouterID(0));
    utility::EcmpSetupAnyNPorts6 v6EcmpHelper(
        getProgrammedState(), RouterID(0));
    v4EcmpHelper.resolveNextHops(getProgrammedState(), 1);
    v6EcmpHelper.resolveNextHops(getProgrammedState(), 1);
    auto resolvedNeighborsState = getProgrammedState();
    // Rollback resolved neighbors
    rollback(StateDelta(origState, getProgrammedState()));
    // Bring back resolved neighboors
    rollback(StateDelta(resolvedNeighborsState, getProgrammedState()));
    // Back to square 1
    rollback(StateDelta(origState, getProgrammedState()));

    EXPECT_EQ(origState, getProgrammedState());
  };
  verifyAcrossWarmBoots([]() {}, verify);
}
} // namespace facebook::fboss
