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
  EXPECT_EQ(state.toFollyDynamic(), stateBack->toFollyDynamic());
}
