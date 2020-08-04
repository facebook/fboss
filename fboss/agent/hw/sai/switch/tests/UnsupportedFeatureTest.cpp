/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/sai/switch/tests/ManagerTestBase.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/SwitchSettings.h"
#include "fboss/agent/state/SwitchState.h"

#include "fboss/agent/types.h"

#include <string>

using namespace facebook::fboss;

TEST_F(ManagerTestBase, checkQcmSupport) {
  auto newState = std::make_shared<SwitchState>();
  auto newSwitchSettings = newState->getSwitchSettings()->clone();
  newSwitchSettings->setQcmEnable(true);
  newState->resetSwitchSettings(newSwitchSettings);
  EXPECT_THROW(applyNewState(newState), FbossError);
}
