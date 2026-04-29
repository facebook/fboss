/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <stdexcept>
#include <string>

#include "fboss/cli/fboss2/commands/config/routing/CmdConfigRouting.h"
#include "fboss/cli/fboss2/session/ConfigSession.h"
#include "fboss/cli/fboss2/test/config/CmdConfigTestBase.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"

using namespace ::testing;

namespace facebook::fboss {

// Seed config mirrors the switchSettings shape from a real FSW production
// agent.conf: FSWs use 13 route counter IDs to track traffic across 13
// named route groups; RSWs default to 0 (feature disabled).
class CmdConfigRoutingTestFixture : public CmdConfigTestBase {
 public:
  CmdConfigRoutingTestFixture()
      : CmdConfigTestBase(
            "fboss_routing_config_test_%%%%-%%%%-%%%%-%%%%",
            R"({
  "sw": {
    "switchSettings": {
      "l2LearningMode": 0,
      "qcmEnable": false,
      "ptpTcEnable": true,
      "l2AgeTimerSeconds": 300,
      "maxRouteCounterIDs": 13,
      "blockNeighbors": [],
      "macAddrsToBlock": [],
      "switchType": 0,
      "exactMatchTableConfigs": []
    }
  }
})") {}

 protected:
  const std::string cmdPrefix_ = "config routing";
};

// =============================================================
// RoutingConfigArgs validation tests
// =============================================================

TEST_F(CmdConfigRoutingTestFixture, argValidation_valid) {
  RoutingConfigArgs a({"max-route-counter-ids", "13"});
  EXPECT_EQ(a.getAttribute(), "max-route-counter-ids");
  EXPECT_EQ(a.getValue(), 13);

  RoutingConfigArgs b({"max-route-counter-ids", "0"});
  EXPECT_EQ(b.getAttribute(), "max-route-counter-ids");
  EXPECT_EQ(b.getValue(), 0);

  RoutingConfigArgs c({"max-route-counter-ids", "1000"});
  EXPECT_EQ(c.getAttribute(), "max-route-counter-ids");
  EXPECT_EQ(c.getValue(), 1000);
}

TEST_F(CmdConfigRoutingTestFixture, argValidation_badArity) {
  EXPECT_THROW(RoutingConfigArgs({}), std::invalid_argument);
  EXPECT_THROW(
      RoutingConfigArgs({"max-route-counter-ids"}), std::invalid_argument);
  EXPECT_THROW(
      RoutingConfigArgs({"max-route-counter-ids", "10", "extra"}),
      std::invalid_argument);
}

TEST_F(CmdConfigRoutingTestFixture, argValidation_unknownAttr) {
  EXPECT_THROW(
      RoutingConfigArgs({"unknown-attr", "10"}), std::invalid_argument);
  EXPECT_THROW(RoutingConfigArgs({"max-routes", "10"}), std::invalid_argument);
  EXPECT_THROW(
      RoutingConfigArgs({"Max-Route-Counter-Ids", "10"}),
      std::invalid_argument);
}

TEST_F(CmdConfigRoutingTestFixture, argValidation_nonInteger) {
  EXPECT_THROW(
      RoutingConfigArgs({"max-route-counter-ids", "abc"}),
      std::invalid_argument);
  EXPECT_THROW(
      RoutingConfigArgs({"max-route-counter-ids", "3.14"}),
      std::invalid_argument);
  EXPECT_THROW(
      RoutingConfigArgs({"max-route-counter-ids", ""}), std::invalid_argument);
}

TEST_F(CmdConfigRoutingTestFixture, argValidation_negative) {
  EXPECT_THROW(
      RoutingConfigArgs({"max-route-counter-ids", "-1"}),
      std::invalid_argument);
  EXPECT_THROW(
      RoutingConfigArgs({"max-route-counter-ids", "-100"}),
      std::invalid_argument);
}

// =============================================================
// queryClient() tests
// =============================================================

TEST_F(CmdConfigRoutingTestFixture, setMaxRouteCounterIds) {
  setupTestableConfigSession(cmdPrefix_, "max-route-counter-ids 20");
  CmdConfigRouting cmd;
  HostInfo hostInfo("testhost");
  RoutingConfigArgs args({"max-route-counter-ids", "20"});

  auto result = cmd.queryClient(hostInfo, args);
  EXPECT_THAT(result, HasSubstr("max-route-counter-ids"));
  EXPECT_THAT(result, HasSubstr("20"));

  auto& config = ConfigSession::getInstance().getAgentConfig();
  EXPECT_EQ(*config.sw()->switchSettings()->maxRouteCounterIDs(), 20);
}

TEST_F(CmdConfigRoutingTestFixture, setMaxRouteCounterIdsToZero) {
  setupTestableConfigSession(cmdPrefix_, "max-route-counter-ids 0");
  CmdConfigRouting cmd;
  HostInfo hostInfo("testhost");
  RoutingConfigArgs args({"max-route-counter-ids", "0"});

  auto result = cmd.queryClient(hostInfo, args);
  EXPECT_THAT(result, HasSubstr("0"));

  auto& config = ConfigSession::getInstance().getAgentConfig();
  EXPECT_EQ(*config.sw()->switchSettings()->maxRouteCounterIDs(), 0);
}

TEST_F(CmdConfigRoutingTestFixture, idempotentSameValue) {
  setupTestableConfigSession(cmdPrefix_, "max-route-counter-ids 13");
  CmdConfigRouting cmd;
  HostInfo hostInfo("testhost");
  RoutingConfigArgs args({"max-route-counter-ids", "13"});

  cmd.queryClient(hostInfo, args);
  cmd.queryClient(hostInfo, args);

  auto& config = ConfigSession::getInstance().getAgentConfig();
  EXPECT_EQ(*config.sw()->switchSettings()->maxRouteCounterIDs(), 13);
}

} // namespace facebook::fboss
