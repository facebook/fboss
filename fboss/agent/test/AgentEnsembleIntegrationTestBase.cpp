// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/test/AgentEnsembleIntegrationTestBase.h"

#include <folly/gen/Base.h>

namespace facebook::fboss {

void AgentEnsembleIntegrationTestBase::SetUp() {
  AgentEnsembleTest::setupAgentEnsemble();
}

void AgentEnsembleIntegrationTestBase::TearDown() {
  dumpRunningConfig(getTestConfigPath());
  AgentEnsembleTest::TearDown();
}
} // namespace facebook::fboss
