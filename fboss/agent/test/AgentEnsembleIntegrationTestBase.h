// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <folly/Synchronized.h>
#include <folly/logging/xlog.h>

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/test/AgentEnsembleTest.h"
#include "fboss/agent/test/utils/AsicUtils.h"

DECLARE_bool(tun_intf);
DECLARE_string(config);

namespace facebook::fboss {

// Base class for sw+hw integration tests that can run on single nodes
class AgentEnsembleIntegrationTestBase : public AgentEnsembleTest {
 public:
  void SetUp() override;
  void TearDown() override;

 private:
  const AgentEnsemble* ensemble_;
};
} // namespace facebook::fboss
