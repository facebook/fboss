// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/test/AgentEnsembleIntegrationTestBase.h"
#include "fboss/agent/hw/test/ConfigFactory.h"

#include <folly/gen/Base.h>

namespace facebook::fboss {

void AgentEnsembleIntegrationTestBase::SetUp() {
  AgentEnsembleTest::setupAgentEnsemble(false);
}

void AgentEnsembleIntegrationTestBase::TearDown() {
  dumpRunningConfig(getTestConfigPath());
  AgentEnsembleTest::TearDown();
}
} // namespace facebook::fboss
