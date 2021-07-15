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
std::vector<facebook::fboss::AclEntryThrift> createAclEntries() {
  facebook::fboss::AclEntryThrift aclEntry1;
  aclEntry1.name_ref() = "cpuPolicing-CPU-Port-Mcast-v6";
  aclEntry1.priority_ref() = 7;
  aclEntry1.actionType_ref() = "permit";

  fboss::AclEntryThrift aclEntry2;
  aclEntry2.name_ref() = "cpuPolicing-high-BGPDstPort-dstLocalIp4";
  aclEntry2.priority_ref() = 2;
  aclEntry2.l4DstPort_ref() = 179;
  aclEntry2.actionType_ref() = "permit";

  fboss::AclEntryThrift aclEntry3;
  aclEntry3.name_ref() = "cpuPolicing-high-slow-protocols-mac";
  aclEntry3.priority_ref() = 14;
  aclEntry3.dstMac_ref() = "01:80:c2:00:00:02";
  aclEntry3.actionType_ref() = "permit";

  return {aclEntry1, aclEntry2, aclEntry3};
}

class CmdShowAclTestFixture : public CmdHandlerTestBase {
 public:
  std::vector<fboss::AclEntryThrift> aclEntries;

  void SetUp() override {
    CmdHandlerTestBase::SetUp();
    aclEntries = createAclEntries();
  }
};

TEST_F(CmdShowAclTestFixture, queryClient) {
  setupMockedAgentServer();
  EXPECT_CALL(getMockAgent(), getAclTable(_))
      .WillOnce(Invoke([&](auto& entries) { entries = aclEntries; }));

  auto cmd = CmdShowAcl();
  auto model = cmd.queryClient(localhost());
  auto entries = model.get_aclEntries();

  EXPECT_EQ(entries.size(), 3);

  EXPECT_EQ(entries[0].get_name(), "cpuPolicing-CPU-Port-Mcast-v6");
  EXPECT_EQ(entries[0].get_priority(), 7);
  EXPECT_EQ(entries[0].get_actionType(), "permit");

  EXPECT_EQ(entries[1].get_name(), "cpuPolicing-high-BGPDstPort-dstLocalIp4");
  EXPECT_EQ(entries[1].get_priority(), 2);
  EXPECT_EQ(entries[1].get_l4DstPort(), 179);
  EXPECT_EQ(entries[1].get_actionType(), "permit");

  EXPECT_EQ(entries[2].get_name(), "cpuPolicing-high-slow-protocols-mac");
  EXPECT_EQ(entries[2].get_priority(), 14);
  EXPECT_EQ(entries[2].get_dstMac(), "01:80:c2:00:00:02");
  EXPECT_EQ(entries[2].get_actionType(), "permit");
}

TEST_F(CmdShowAclTestFixture, printOutput) {
  auto cmd = CmdShowAcl();
  auto model = cmd.createModel(aclEntries);

  std::stringstream ss;
  cmd.printOutput(model, ss);

  std::string output = ss.str();
  std::string expectOutput =
      R"(Acl: cpuPolicing-CPU-Port-Mcast-v6
   priority: 7
   action: permit

Acl: cpuPolicing-high-BGPDstPort-dstLocalIp4
   priority: 2
   L4 dst port: 179
   action: permit

Acl: cpuPolicing-high-slow-protocols-mac
   priority: 14
   dst mac: 01:80:c2:00:00:02
   action: permit

)";
  EXPECT_EQ(output, expectOutput);
}
} // namespace facebook::fboss
