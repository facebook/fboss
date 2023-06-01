/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/Main.h"
#include "fboss/agent/test/AgentHwTest.h"

DECLARE_string(config);

namespace facebook::fboss {
class AgentIntegrationTest : public AgentHwTest {
 protected:
  void SetUp() override;
  cfg::SwitchConfig initialConfig() const override;
};

int agentIntegrationTestMain(
    int argc,
    char** argv,
    facebook::fboss::PlatformInitFn initPlatformFn);

} // namespace facebook::fboss
