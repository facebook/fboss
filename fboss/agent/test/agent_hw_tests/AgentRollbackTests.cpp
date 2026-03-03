/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/test/agent_hw_tests/AgentRollbackTests.h"

#include "fboss/agent/test/utils/ConfigUtils.h"

namespace facebook::fboss {

cfg::SwitchConfig AgentRollbackTest::initialConfig(
    const AgentEnsemble& ensemble) const {
  return utility::onePortPerInterfaceConfig(
      ensemble.getSw(),
      ensemble.masterLogicalPortIds(),
      true /*interfaceHasSubnet*/);
}

void AgentRollbackTest::setCmdLineFlagOverrides() const {
  AgentHwTest::setCmdLineFlagOverrides();
}

void AgentRollbackTest::SetUp() {
  AgentHwTest::SetUp();
}

} // namespace facebook::fboss
