// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <folly/IPAddressV4.h>
#include "fboss/cli/fboss2/commands/show/acl/CmdShowAcl.h"
#include "fboss/cli/fboss2/commands/show/acl/gen-cpp2/model_types.h"
#include "fboss/cli/fboss2/test/CmdHandlerTestBase.h"

using namespace ::testing;

namespace facebook::fboss {

/*
 * Set up test data
 */
std::vector<facebook::fboss::AclEntryThrift> createAclEntries(
    std::string aclTableName) {
  facebook::fboss::AclEntryThrift aclEntry1;
  aclEntry1.name() =
      folly::to<std::string>(aclTableName, "_cpuPolicing-CPU-Port-Mcast-v6");
  aclEntry1.priority() = 7;
  aclEntry1.actionType() = "permit";

  fboss::AclEntryThrift aclEntry2;
  aclEntry2.name() = folly::to<std::string>(
      aclTableName, "_cpuPolicing-high-BGPDstPort-dstLocalIp4");
  aclEntry2.priority() = 2;
  aclEntry2.l4DstPort() = 179;
  aclEntry2.actionType() = "permit";

  fboss::AclEntryThrift aclEntry3;
  aclEntry3.name() = folly::to<std::string>(
      aclTableName, "_cpuPolicing-high-slow-protocols-mac");
  aclEntry3.priority() = 14;
  aclEntry3.dstMac() = "01:80:c2:00:00:02";
  aclEntry3.actionType() = "permit";

  return {aclEntry1, aclEntry2, aclEntry3};
}

std::map<std::string, std::vector<AclEntryThrift>> createAclTableEntries() {
  std::map<std::string, std::vector<AclEntryThrift>> aclTableEntries;
  aclTableEntries["aclTable1"] = createAclEntries("aclTable1");
  aclTableEntries["aclTable2"] = createAclEntries("aclTable2");
  return aclTableEntries;
}

class CmdShowAclTestFixture : public CmdHandlerTestBase {
 public:
  AclTableThrift aclTableEntries;

  void SetUp() override {
    CmdHandlerTestBase::SetUp();
    aclTableEntries.aclTableEntries() = createAclTableEntries();
  }
};

TEST_F(CmdShowAclTestFixture, queryClient) {
  setupMockedAgentServer();
  EXPECT_CALL(getMockAgent(), getAclTableGroup(_))
      .WillOnce(Invoke([&](auto& entries) { entries = aclTableEntries; }));

  auto cmd = CmdShowAcl();
  auto model = cmd.queryClient(localhost());
  auto aclTableEntries = model.aclTableEntries();
  int tableNum = 1;
  for (auto& [aclTableName, entries] : *aclTableEntries) {
    auto expectedAclTableName = folly::to<std::string>("aclTable", tableNum++);
    EXPECT_EQ(aclTableName, expectedAclTableName);

    EXPECT_EQ(entries.size(), 3);

    EXPECT_EQ(
        entries[0].get_name(),
        folly::to<std::string>(
            expectedAclTableName, "_cpuPolicing-CPU-Port-Mcast-v6"));
    EXPECT_EQ(entries[0].get_priority(), 7);
    EXPECT_EQ(entries[0].get_actionType(), "permit");

    EXPECT_EQ(
        entries[1].get_name(),
        folly::to<std::string>(
            expectedAclTableName, "_cpuPolicing-high-BGPDstPort-dstLocalIp4"));
    EXPECT_EQ(entries[1].get_priority(), 2);
    EXPECT_EQ(entries[1].get_l4DstPort(), 179);
    EXPECT_EQ(entries[1].get_actionType(), "permit");

    EXPECT_EQ(
        entries[2].get_name(),
        folly::to<std::string>(
            expectedAclTableName, "_cpuPolicing-high-slow-protocols-mac"));
    EXPECT_EQ(entries[2].get_priority(), 14);
    EXPECT_EQ(entries[2].get_dstMac(), "01:80:c2:00:00:02");
    EXPECT_EQ(entries[2].get_actionType(), "permit");
  }
}

TEST_F(CmdShowAclTestFixture, printOutput) {
  auto cmd = CmdShowAcl();
  auto model = cmd.createModel(aclTableEntries);

  std::stringstream ss;
  cmd.printOutput(model, ss);

  std::string output = ss.str();
  std::string expectOutput =
      R"(ACL Table Name: aclTable1
Acl: aclTable1_cpuPolicing-CPU-Port-Mcast-v6
   priority: 7
   action: permit

Acl: aclTable1_cpuPolicing-high-BGPDstPort-dstLocalIp4
   priority: 2
   L4 dst port: 179
   action: permit

Acl: aclTable1_cpuPolicing-high-slow-protocols-mac
   priority: 14
   dst mac: 01:80:c2:00:00:02
   action: permit

ACL Table Name: aclTable2
Acl: aclTable2_cpuPolicing-CPU-Port-Mcast-v6
   priority: 7
   action: permit

Acl: aclTable2_cpuPolicing-high-BGPDstPort-dstLocalIp4
   priority: 2
   L4 dst port: 179
   action: permit

Acl: aclTable2_cpuPolicing-high-slow-protocols-mac
   priority: 14
   dst mac: 01:80:c2:00:00:02
   action: permit

)";
  EXPECT_EQ(output, expectOutput);
}
} // namespace facebook::fboss
