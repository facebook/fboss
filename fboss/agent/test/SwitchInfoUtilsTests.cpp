/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <gtest/gtest.h>

#include "fboss/agent/AgentConfig.h"
#include "fboss/agent/SwitchInfoUtils.h"
#include "fboss/agent/gen-cpp2/agent_config_types.h"
#include "fboss/agent/gen-cpp2/platform_config_types.h"

#include <folly/FileUtil.h>
#include <folly/ScopeGuard.h>
#include <folly/testing/TestUtil.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>

DECLARE_string(config);

using namespace facebook::fboss;

namespace {

constexpr auto kPortId = 1;
constexpr auto kPortName = "eth1/1/1";

cfg::AgentConfig createAgentConfigWithPortAssignment() {
  cfg::AgentConfig config;
  cfg::PortAssignment assignment;
  assignment.portName() = kPortName;
  config.platform()->portIdToPortAssignment() = {{kPortId, assignment}};
  return config;
}

} // namespace

TEST(SwitchInfoUtilsTest, PlatformConfigFromAgentConfig) {
  auto agentConfig = createAgentConfigWithPortAssignment();
  auto config = std::make_unique<AgentConfig>(agentConfig);

  EXPECT_EQ(getPlatformConfigFromConfig(config.get()), *agentConfig.platform());
}

// SwSwitch is constructed without an AgentConfig in multi switch mode, in
// which case the platform config has to be read back from FLAGS_config.
TEST(SwitchInfoUtilsTest, PlatformConfigFromNullConfigReadsDefaultFile) {
  folly::test::TemporaryDirectory tmpDir;
  auto configPath = (tmpDir.path() / "agent.conf").string();
  auto agentConfig = createAgentConfigWithPortAssignment();
  ASSERT_TRUE(
      folly::writeFile(
          apache::thrift::SimpleJSONSerializer::serialize<std::string>(
              agentConfig),
          configPath.c_str()));

  auto savedConfigPath = FLAGS_config;
  SCOPE_EXIT {
    FLAGS_config = savedConfigPath;
  };
  FLAGS_config = configPath;

  EXPECT_EQ(getPlatformConfigFromConfig(nullptr), *agentConfig.platform());
}

TEST(SwitchInfoUtilsTest, PlatformConfigFromNullConfigWithoutDefaultFile) {
  auto savedConfigPath = FLAGS_config;
  SCOPE_EXIT {
    FLAGS_config = savedConfigPath;
  };
  FLAGS_config = "/non/existent/agent.conf";

  EXPECT_EQ(getPlatformConfigFromConfig(nullptr), cfg::PlatformConfig());
}
