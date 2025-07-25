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
#include "fboss/agent/test/AgentEnsembleIntegrationTestBase.h"

DECLARE_string(config);

namespace facebook::fboss {
class AgentIntegrationTest : public AgentEnsembleIntegrationTestBase {
 protected:
  void SetUp() override;
  cfg::SwitchConfig initialConfig(const AgentEnsemble& ensemble) override;
};

int agentIntegrationTestMain(
    int argc,
    char** argv,
    facebook::fboss::PlatformInitFn initPlatformFn,
    std::optional<cfg::StreamType> streamType = std::nullopt);

} // namespace facebook::fboss
