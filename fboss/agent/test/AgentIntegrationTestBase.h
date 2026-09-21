// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <folly/Synchronized.h>
#include <folly/logging/xlog.h>

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/test/AgentTest.h"

DECLARE_bool(tun_intf);
DECLARE_string(config);

namespace facebook::fboss {

// Base class for sw+hw integration tests that can run on single nodes
class AgentIntegrationTestBase : public AgentTest {
 public:
  void SetUp() override;
  void TearDown() override;

  const std::vector<PortID>& masterLogicalPortIds() const {
    return masterLogicalPortIds_;
  }

  void setupConfigFlag() override;
  virtual cfg::SwitchConfig initialConfig() const = 0;

 private:
  std::vector<PortID> masterLogicalPortIds_;
};
} // namespace facebook::fboss
