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

#include "fboss/cli/fboss2/commands/config/need_l2_entry_for_neighbor/CmdConfigNeedL2EntryForNeighbor.h"
#include "fboss/cli/fboss2/session/ConfigSession.h"
#include "fboss/cli/fboss2/test/config/CmdConfigTestBase.h"

using namespace ::testing;

namespace facebook::fboss {

class CmdConfigNeedL2EntryForNeighborTestFixture : public CmdConfigTestBase {
 public:
  CmdConfigNeedL2EntryForNeighborTestFixture()
      : CmdConfigTestBase(
            "fboss_need_l2_test_%%%%-%%%%-%%%%-%%%%",
            R"({
  "sw": {
    "switchSettings": {
      "needL2EntryForNeighbor": true
    }
  }
})") {}

 protected:
  const std::string cmdPrefix_ = "config need-l2-entry-for-neighbor";
};

// ==============================================================================
// NeedL2EntryForNeighborArg Validation Tests
// ==============================================================================

TEST_F(CmdConfigNeedL2EntryForNeighborTestFixture, argValidationEnabled) {
  EXPECT_TRUE(NeedL2EntryForNeighborArg({"enabled"}).isEnabled());
  EXPECT_TRUE(NeedL2EntryForNeighborArg({"ENABLED"}).isEnabled());
  EXPECT_TRUE(NeedL2EntryForNeighborArg({"Enabled"}).isEnabled());
}

TEST_F(CmdConfigNeedL2EntryForNeighborTestFixture, argValidationDisabled) {
  EXPECT_FALSE(NeedL2EntryForNeighborArg({"disabled"}).isEnabled());
  EXPECT_FALSE(NeedL2EntryForNeighborArg({"DISABLED"}).isEnabled());
  EXPECT_FALSE(NeedL2EntryForNeighborArg({"Disabled"}).isEnabled());
}

TEST_F(CmdConfigNeedL2EntryForNeighborTestFixture, argValidationRejectsEmpty) {
  EXPECT_THROW(NeedL2EntryForNeighborArg({}), std::invalid_argument);
}

TEST_F(
    CmdConfigNeedL2EntryForNeighborTestFixture,
    argValidationRejectsExtraArgs) {
  EXPECT_THROW(
      NeedL2EntryForNeighborArg({"enabled", "extra"}), std::invalid_argument);
}

TEST_F(
    CmdConfigNeedL2EntryForNeighborTestFixture,
    argValidationRejectsInvalid) {
  EXPECT_THROW(NeedL2EntryForNeighborArg({"true"}), std::invalid_argument);
  EXPECT_THROW(NeedL2EntryForNeighborArg({"yes"}), std::invalid_argument);
  EXPECT_THROW(NeedL2EntryForNeighborArg({"1"}), std::invalid_argument);
}

// ==============================================================================
// Command Execution Tests
// ==============================================================================

TEST_F(CmdConfigNeedL2EntryForNeighborTestFixture, setToDisabled) {
  setupTestableConfigSession(cmdPrefix_, "disabled");
  CmdConfigNeedL2EntryForNeighbor cmd;
  HostInfo hostInfo("testhost");
  NeedL2EntryForNeighborArg arg({"disabled"});

  auto result = cmd.queryClient(hostInfo, arg);
  EXPECT_THAT(result, HasSubstr("disabled"));

  auto& config = ConfigSession::getInstance().getAgentConfig();
  auto value = config.sw()->switchSettings()->needL2EntryForNeighbor();
  ASSERT_TRUE(value.has_value());
  EXPECT_FALSE(*value);
}

TEST_F(CmdConfigNeedL2EntryForNeighborTestFixture, setToEnabled) {
  setupTestableConfigSession(cmdPrefix_, "disabled");
  CmdConfigNeedL2EntryForNeighbor cmd;
  HostInfo hostInfo("testhost");

  // First disable it
  cmd.queryClient(hostInfo, NeedL2EntryForNeighborArg({"disabled"}));

  // Then re-enable
  auto result =
      cmd.queryClient(hostInfo, NeedL2EntryForNeighborArg({"enabled"}));
  EXPECT_THAT(result, HasSubstr("enabled"));

  auto& config = ConfigSession::getInstance().getAgentConfig();
  auto value = config.sw()->switchSettings()->needL2EntryForNeighbor();
  ASSERT_TRUE(value.has_value());
  EXPECT_TRUE(*value);
}

TEST_F(CmdConfigNeedL2EntryForNeighborTestFixture, alreadySetReturnsMessage) {
  setupTestableConfigSession(cmdPrefix_, "enabled");
  CmdConfigNeedL2EntryForNeighbor cmd;
  HostInfo hostInfo("testhost");

  // Seed JSON has needL2EntryForNeighbor: true
  auto result =
      cmd.queryClient(hostInfo, NeedL2EntryForNeighborArg({"enabled"}));
  EXPECT_THAT(result, HasSubstr("already"));
}

} // namespace facebook::fboss
