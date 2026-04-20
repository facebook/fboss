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

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/cli/fboss2/commands/config/acl/table/CmdConfigAclTable.h"
#include "fboss/cli/fboss2/commands/config/acl/table_group/CmdConfigAclTableGroup.h"
#include "fboss/cli/fboss2/session/ConfigSession.h"
#include "fboss/cli/fboss2/test/config/CmdConfigTestBase.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"

using namespace ::testing;

namespace facebook::fboss {

// Seed mirrors an RSW device's aclTableGroups (simplified subset). Two
// groups, the first with two tables, so queryClient() tests can assert that
// mutating the targeted group/table leaves its siblings untouched -- with
// only one group/table, a `find_if` bug that matched "first element" instead
// of "match by name" would pass undetected.
static constexpr auto kSeedConfig = R"({
  "sw": {
    "aclTableGroups": [{
      "name": "ingress-ACL-Table-Group",
      "stage": 0,
      "aclTables": [{
        "name": "AclTable1",
        "priority": 0,
        "aclEntries": [],
        "actionTypes": [],
        "qualifiers": [],
        "udfGroups": []
      }, {
        "name": "AclTable2",
        "priority": 1,
        "aclEntries": [],
        "actionTypes": [],
        "qualifiers": [],
        "udfGroups": []
      }]
    }, {
      "name": "egress-ACL-Table-Group",
      "stage": 2,
      "aclTables": [{
        "name": "AclTable3",
        "priority": 0,
        "aclEntries": [],
        "actionTypes": [],
        "qualifiers": [],
        "udfGroups": []
      }]
    }]
  }
})";

// Seed with no aclTableGroups at all, for exercising the "No aclTableGroups
// found" error branch shared by both queryClient() implementations.
static constexpr auto kSeedConfigNoAclTableGroups = R"({
  "sw": {}
})";

class CmdConfigAclTableGroupTestFixture : public CmdConfigTestBase {
 public:
  CmdConfigAclTableGroupTestFixture()
      : CmdConfigTestBase(
            "fboss_acl_table_group_config_test_%%%%-%%%%-%%%%-%%%%",
            kSeedConfig) {}
};

class CmdConfigAclTableTestFixture : public CmdConfigTestBase {
 public:
  CmdConfigAclTableTestFixture()
      : CmdConfigTestBase(
            "fboss_acl_table_config_test_%%%%-%%%%-%%%%-%%%%",
            kSeedConfig) {}
};

class CmdConfigAclNoGroupsTestFixture : public CmdConfigTestBase {
 public:
  CmdConfigAclNoGroupsTestFixture()
      : CmdConfigTestBase(
            "fboss_acl_no_groups_config_test_%%%%-%%%%-%%%%-%%%%",
            kSeedConfigNoAclTableGroups) {}
};

// =============================================================
// AclTableGroupConfigArgs validation tests
// =============================================================

TEST_F(CmdConfigAclTableGroupTestFixture, argValidation_valid) {
  AclTableGroupConfigArgs a({"ingress-ACL-Table-Group", "stage", "ingress"});
  EXPECT_EQ(a.getGroupName(), "ingress-ACL-Table-Group");
  EXPECT_EQ(a.getStage(), cfg::AclStage::INGRESS);

  AclTableGroupConfigArgs b(
      {"ingress-ACL-Table-Group", "stage", "ingress-macsec"});
  EXPECT_EQ(b.getStage(), cfg::AclStage::INGRESS_MACSEC);

  AclTableGroupConfigArgs c(
      {"ingress-ACL-Table-Group", "stage", "egress-macsec"});
  EXPECT_EQ(c.getStage(), cfg::AclStage::EGRESS_MACSEC);

  AclTableGroupConfigArgs d(
      {"ingress-ACL-Table-Group", "stage", "ingress-post-lookup"});
  EXPECT_EQ(d.getStage(), cfg::AclStage::INGRESS_POST_LOOKUP);

  // Numeric fallback
  AclTableGroupConfigArgs e({"ingress-ACL-Table-Group", "stage", "0"});
  EXPECT_EQ(e.getStage(), cfg::AclStage::INGRESS);

  AclTableGroupConfigArgs f({"ingress-ACL-Table-Group", "stage", "3"});
  EXPECT_EQ(f.getStage(), cfg::AclStage::INGRESS_POST_LOOKUP);
}

TEST_F(CmdConfigAclTableGroupTestFixture, argValidation_badArity) {
  EXPECT_THROW(AclTableGroupConfigArgs({}), std::invalid_argument);
  EXPECT_THROW(
      AclTableGroupConfigArgs({"ingress-ACL-Table-Group"}),
      std::invalid_argument);
  EXPECT_THROW(
      AclTableGroupConfigArgs({"ingress-ACL-Table-Group", "stage"}),
      std::invalid_argument);
  EXPECT_THROW(
      AclTableGroupConfigArgs(
          {"ingress-ACL-Table-Group", "stage", "ingress", "extra"}),
      std::invalid_argument);
}

TEST_F(CmdConfigAclTableGroupTestFixture, argValidation_unknownAttr) {
  EXPECT_THROW(
      AclTableGroupConfigArgs(
          {"ingress-ACL-Table-Group", "priority", "ingress"}),
      std::invalid_argument);
}

TEST_F(CmdConfigAclTableGroupTestFixture, argValidation_invalidStage) {
  EXPECT_THROW(
      AclTableGroupConfigArgs({"ingress-ACL-Table-Group", "stage", "egress"}),
      std::invalid_argument);
  EXPECT_THROW(
      AclTableGroupConfigArgs({"ingress-ACL-Table-Group", "stage", "4"}),
      std::invalid_argument);
  EXPECT_THROW(
      AclTableGroupConfigArgs({"ingress-ACL-Table-Group", "stage", "abc"}),
      std::invalid_argument);
}

// =============================================================
// queryClient() tests for table-group
// =============================================================

TEST_F(CmdConfigAclTableGroupTestFixture, setStageIngressPostLookup) {
  setupTestableConfigSession(
      "config acl table-group",
      "ingress-ACL-Table-Group stage ingress-post-lookup");
  CmdConfigAclTableGroup cmd;
  HostInfo hostInfo("testhost");
  AclTableGroupConfigArgs args(
      {"ingress-ACL-Table-Group", "stage", "ingress-post-lookup"});

  auto result = cmd.queryClient(hostInfo, args);
  EXPECT_THAT(result, HasSubstr("ingress-ACL-Table-Group"));
  EXPECT_THAT(result, HasSubstr("stage"));

  auto& config = ConfigSession::getInstance().getAgentConfig();
  auto& groups = *config.sw()->aclTableGroups();
  ASSERT_EQ(groups.size(), 2u);
  EXPECT_EQ(*groups[0].name(), "ingress-ACL-Table-Group");
  EXPECT_EQ(*groups[0].stage(), cfg::AclStage::INGRESS_POST_LOOKUP);
  // The sibling group must be untouched.
  EXPECT_EQ(*groups[1].name(), "egress-ACL-Table-Group");
  EXPECT_EQ(*groups[1].stage(), cfg::AclStage::EGRESS_MACSEC);
}

TEST_F(CmdConfigAclTableGroupTestFixture, setStageNoopSameValue) {
  setupTestableConfigSession(
      "config acl table-group", "ingress-ACL-Table-Group stage ingress");
  CmdConfigAclTableGroup cmd;
  HostInfo hostInfo("testhost");
  AclTableGroupConfigArgs args({"ingress-ACL-Table-Group", "stage", "ingress"});

  cmd.queryClient(hostInfo, args);
  cmd.queryClient(hostInfo, args);

  auto& config = ConfigSession::getInstance().getAgentConfig();
  EXPECT_EQ(
      *config.sw()->aclTableGroups()->at(0).stage(), cfg::AclStage::INGRESS);
  EXPECT_EQ(
      *config.sw()->aclTableGroups()->at(1).stage(),
      cfg::AclStage::EGRESS_MACSEC);
}

// Targets the second (non-first) group by name, so a lookup bug that always
// matched the first element regardless of name would be caught here (the
// tests above only ever mutate index 0, which such a bug would pass).
TEST_F(CmdConfigAclTableGroupTestFixture, setStageOnNonFirstGroup) {
  setupTestableConfigSession(
      "config acl table-group", "egress-ACL-Table-Group stage ingress");
  CmdConfigAclTableGroup cmd;
  HostInfo hostInfo("testhost");
  AclTableGroupConfigArgs args({"egress-ACL-Table-Group", "stage", "ingress"});

  auto result = cmd.queryClient(hostInfo, args);
  EXPECT_THAT(result, HasSubstr("egress-ACL-Table-Group"));

  auto& config = ConfigSession::getInstance().getAgentConfig();
  auto& groups = *config.sw()->aclTableGroups();
  ASSERT_EQ(groups.size(), 2u);
  // The first group must be untouched.
  EXPECT_EQ(*groups[0].name(), "ingress-ACL-Table-Group");
  EXPECT_EQ(*groups[0].stage(), cfg::AclStage::INGRESS);
  EXPECT_EQ(*groups[1].name(), "egress-ACL-Table-Group");
  EXPECT_EQ(*groups[1].stage(), cfg::AclStage::INGRESS);
}

TEST_F(CmdConfigAclTableGroupTestFixture, groupNotFound) {
  setupTestableConfigSession(
      "config acl table-group", "no-such-group stage ingress");
  CmdConfigAclTableGroup cmd;
  HostInfo hostInfo("testhost");
  AclTableGroupConfigArgs args({"no-such-group", "stage", "ingress"});

  EXPECT_THROW(cmd.queryClient(hostInfo, args), std::runtime_error);
}

TEST_F(CmdConfigAclNoGroupsTestFixture, tableGroupNoAclTableGroupsConfigured) {
  setupTestableConfigSession(
      "config acl table-group", "ingress-ACL-Table-Group stage ingress");
  CmdConfigAclTableGroup cmd;
  HostInfo hostInfo("testhost");
  AclTableGroupConfigArgs args({"ingress-ACL-Table-Group", "stage", "ingress"});

  EXPECT_THROW(cmd.queryClient(hostInfo, args), std::runtime_error);
}

// =============================================================
// AclTableConfigArgs validation tests
// =============================================================

TEST_F(CmdConfigAclTableTestFixture, argValidation_valid) {
  AclTableConfigArgs a(
      {"ingress-ACL-Table-Group", "AclTable1", "priority", "5"});
  EXPECT_EQ(a.getGroupName(), "ingress-ACL-Table-Group");
  EXPECT_EQ(a.getTableName(), "AclTable1");
  EXPECT_EQ(a.getPriority(), 5);

  AclTableConfigArgs b(
      {"ingress-ACL-Table-Group", "AclTable1", "priority", "0"});
  EXPECT_EQ(b.getPriority(), 0);
}

TEST_F(CmdConfigAclTableTestFixture, argValidation_badArity) {
  EXPECT_THROW(AclTableConfigArgs({}), std::invalid_argument);
  EXPECT_THROW(
      AclTableConfigArgs({"ingress-ACL-Table-Group"}), std::invalid_argument);
  EXPECT_THROW(
      AclTableConfigArgs({"ingress-ACL-Table-Group", "AclTable1"}),
      std::invalid_argument);
  EXPECT_THROW(
      AclTableConfigArgs({"ingress-ACL-Table-Group", "AclTable1", "priority"}),
      std::invalid_argument);
  EXPECT_THROW(
      AclTableConfigArgs(
          {"ingress-ACL-Table-Group", "AclTable1", "priority", "1", "extra"}),
      std::invalid_argument);
}

TEST_F(CmdConfigAclTableTestFixture, argValidation_unknownAttr) {
  EXPECT_THROW(
      AclTableConfigArgs(
          {"ingress-ACL-Table-Group", "AclTable1", "stage", "1"}),
      std::invalid_argument);
}

TEST_F(CmdConfigAclTableTestFixture, argValidation_nonInteger) {
  EXPECT_THROW(
      AclTableConfigArgs(
          {"ingress-ACL-Table-Group", "AclTable1", "priority", "abc"}),
      std::invalid_argument);
  EXPECT_THROW(
      AclTableConfigArgs(
          {"ingress-ACL-Table-Group", "AclTable1", "priority", "3.14"}),
      std::invalid_argument);
}

TEST_F(CmdConfigAclTableTestFixture, argValidation_negative) {
  EXPECT_THROW(
      AclTableConfigArgs(
          {"ingress-ACL-Table-Group", "AclTable1", "priority", "-1"}),
      std::invalid_argument);
}

// =============================================================
// queryClient() tests for table
// =============================================================

TEST_F(CmdConfigAclTableTestFixture, setPriority) {
  setupTestableConfigSession(
      "config acl table", "ingress-ACL-Table-Group AclTable1 priority 10");
  CmdConfigAclTable cmd;
  HostInfo hostInfo("testhost");
  AclTableConfigArgs args(
      {"ingress-ACL-Table-Group", "AclTable1", "priority", "10"});

  auto result = cmd.queryClient(hostInfo, args);
  EXPECT_THAT(result, HasSubstr("AclTable1"));
  EXPECT_THAT(result, HasSubstr("priority"));
  EXPECT_THAT(result, HasSubstr("10"));

  auto& config = ConfigSession::getInstance().getAgentConfig();
  auto& groups = *config.sw()->aclTableGroups();
  ASSERT_EQ(groups.size(), 2u);
  auto& tables = *groups[0].aclTables();
  ASSERT_EQ(tables.size(), 2u);
  EXPECT_EQ(*tables[0].name(), "AclTable1");
  EXPECT_EQ(*tables[0].priority(), 10);
  // The sibling table in the same group must be untouched.
  EXPECT_EQ(*tables[1].name(), "AclTable2");
  EXPECT_EQ(*tables[1].priority(), 1);
  // The table in the sibling group must also be untouched.
  auto& otherGroupTables = *groups[1].aclTables();
  ASSERT_EQ(otherGroupTables.size(), 1u);
  EXPECT_EQ(*otherGroupTables[0].priority(), 0);
}

// Targets the second (non-first) table by name, within a group that also
// isn't first, so a lookup bug that always matched the first element
// regardless of name would be caught here (setPriority above only ever
// mutates index 0 of both the group and table lists).
TEST_F(CmdConfigAclTableTestFixture, setPriorityOnNonFirstTable) {
  setupTestableConfigSession(
      "config acl table", "ingress-ACL-Table-Group AclTable2 priority 20");
  CmdConfigAclTable cmd;
  HostInfo hostInfo("testhost");
  AclTableConfigArgs args(
      {"ingress-ACL-Table-Group", "AclTable2", "priority", "20"});

  auto result = cmd.queryClient(hostInfo, args);
  EXPECT_THAT(result, HasSubstr("AclTable2"));

  auto& config = ConfigSession::getInstance().getAgentConfig();
  auto& tables = *config.sw()->aclTableGroups()->at(0).aclTables();
  ASSERT_EQ(tables.size(), 2u);
  // The first table must be untouched.
  EXPECT_EQ(*tables[0].name(), "AclTable1");
  EXPECT_EQ(*tables[0].priority(), 0);
  EXPECT_EQ(*tables[1].name(), "AclTable2");
  EXPECT_EQ(*tables[1].priority(), 20);
}

TEST_F(CmdConfigAclTableTestFixture, tableNotFound) {
  setupTestableConfigSession(
      "config acl table", "ingress-ACL-Table-Group no-such-table priority 1");
  CmdConfigAclTable cmd;
  HostInfo hostInfo("testhost");
  AclTableConfigArgs args(
      {"ingress-ACL-Table-Group", "no-such-table", "priority", "1"});

  EXPECT_THROW(cmd.queryClient(hostInfo, args), std::runtime_error);
}

TEST_F(CmdConfigAclTableTestFixture, groupNotFound) {
  setupTestableConfigSession(
      "config acl table", "no-such-group AclTable1 priority 1");
  CmdConfigAclTable cmd;
  HostInfo hostInfo("testhost");
  AclTableConfigArgs args({"no-such-group", "AclTable1", "priority", "1"});

  EXPECT_THROW(cmd.queryClient(hostInfo, args), std::runtime_error);
}

TEST_F(CmdConfigAclNoGroupsTestFixture, tableNoAclTableGroupsConfigured) {
  setupTestableConfigSession(
      "config acl table", "ingress-ACL-Table-Group AclTable1 priority 1");
  CmdConfigAclTable cmd;
  HostInfo hostInfo("testhost");
  AclTableConfigArgs args(
      {"ingress-ACL-Table-Group", "AclTable1", "priority", "1"});

  EXPECT_THROW(cmd.queryClient(hostInfo, args), std::runtime_error);
}

} // namespace facebook::fboss
