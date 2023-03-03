// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "fboss/cli/fboss2/commands/show/l2/CmdShowL2.h"
#include "fboss/cli/fboss2/commands/show/l2/gen-cpp2/model_types.h"
#include "fboss/cli/fboss2/test/CmdHandlerTestBase.h"

using namespace ::testing;

namespace facebook::fboss {

std::vector<facebook::fboss::L2EntryThrift> createL2Entries() {
  facebook::fboss::L2EntryThrift l2Entry1;
  l2Entry1.mac() = "02:90:fb:64:78:d2";
  l2Entry1.port() = 45;
  l2Entry1.vlanID() = 1024;
  l2Entry1.l2EntryType() = L2EntryType::L2_ENTRY_TYPE_VALIDATED;

  fboss::L2EntryThrift l2Entry2;
  l2Entry2.mac() = "2e:dd:e9:88:b3:db";
  l2Entry2.port() = 36;
  l2Entry2.vlanID() = 3476;
  l2Entry2.trunk() = 1;
  l2Entry2.l2EntryType() = L2EntryType::L2_ENTRY_TYPE_VALIDATED;

  fboss::L2EntryThrift l2Entry3;
  l2Entry3.mac() = "96:8e:d3:dd:0e:a2";
  l2Entry3.port() = 12;
  l2Entry3.vlanID() = 8649;
  l2Entry3.l2EntryType() = L2EntryType::L2_ENTRY_TYPE_PENDING;
  l2Entry3.classID() = 1;

  return {l2Entry1, l2Entry2, l2Entry3};
}

class CmdShowL2TestFixture : public CmdHandlerTestBase {
 public:
  std::vector<fboss::L2EntryThrift> l2Entries;

  void SetUp() override {
    CmdHandlerTestBase::SetUp();
    l2Entries = createL2Entries();
  }
};

TEST_F(CmdShowL2TestFixture, queryClient) {
  setupMockedAgentServer();
  EXPECT_CALL(getMockAgent(), getL2Table(_))
      .WillOnce(Invoke([&](auto& entries) { entries = l2Entries; }));

  auto cmd = CmdShowL2();
  auto model = cmd.queryClient(localhost());
  auto entries = model.get_l2Entries();

  EXPECT_EQ(entries.size(), 3);

  EXPECT_EQ(entries[0].get_mac(), "02:90:fb:64:78:d2");
  EXPECT_EQ(entries[0].get_port(), 45);
  EXPECT_EQ(entries[0].get_vlanID(), 1024);
  EXPECT_EQ(entries[0].get_trunk(), "-");
  EXPECT_EQ(entries[0].get_type(), "Validated");
  EXPECT_EQ(entries[0].get_classID(), "-");

  EXPECT_EQ(entries[1].get_mac(), "2e:dd:e9:88:b3:db");
  EXPECT_EQ(entries[1].get_port(), 36);
  EXPECT_EQ(entries[1].get_vlanID(), 3476);
  EXPECT_EQ(entries[1].get_trunk(), "1");
  EXPECT_EQ(entries[1].get_type(), "Validated");
  EXPECT_EQ(entries[1].get_classID(), "-");

  EXPECT_EQ(entries[2].get_mac(), "96:8e:d3:dd:0e:a2");
  EXPECT_EQ(entries[2].get_port(), 12);
  EXPECT_EQ(entries[2].get_vlanID(), 8649);
  EXPECT_EQ(entries[2].get_trunk(), "-");
  EXPECT_EQ(entries[2].get_type(), "Pending");
  EXPECT_EQ(entries[2].get_classID(), "1");
}

TEST_F(CmdShowL2TestFixture, printOutput) {
  auto cmd = CmdShowL2();
  auto model = cmd.createModel(l2Entries);

  std::stringstream ss;
  cmd.printOutput(model, ss);

  std::string output = ss.str();
  std::string expectOutput =
      " MAC Address        Port  Trunk  VLAN  Type       Class ID \n"
      "------------------------------------------------------------------\n"
      " 02:90:fb:64:78:d2  45    -      1024  Validated  -        \n"
      " 2e:dd:e9:88:b3:db  36    1      3476  Validated  -        \n"
      " 96:8e:d3:dd:0e:a2  12    -      8649  Pending    1        \n\n";
  EXPECT_EQ(output, expectOutput);
}

} // namespace facebook::fboss
