// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <configerator/structs/neteng/fboss/bgp/gen-cpp2/bgp_config_types.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

#include "fboss/cli/fboss2/commands/config/protocol/bgp/peer-group/CmdConfigProtocolBgpPeerGroup.h"
#include "fboss/cli/fboss2/commands/delete/protocol/bgp/peer-group/CmdDeleteProtocolBgpPeerGroup.h"
#include "fboss/cli/fboss2/session/ConfigSession.h"
#include "fboss/cli/fboss2/test/config/CmdConfigTestBase.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"

using namespace ::testing;

namespace facebook::fboss {

// Deleting only touches the BGP side of ConfigSession, which seeds from
// thrift schema defaults when neither a staged session nor a system
// bgpcpp.conf exists — so no seed agent config is needed (mirrors
// CmdConfigBgpPeerGroupTest).
class CmdDeleteBgpPeerGroupTestFixture : public CmdConfigTestBase {
 public:
  CmdDeleteBgpPeerGroupTestFixture()
      : CmdConfigTestBase("bgp_peer_group_delete_test_%%%%-%%%%-%%%%", "") {}

  void SetUp() override {
    CmdConfigTestBase::SetUp();
    setupTestableConfigSession();
  }

  std::string configure(const std::vector<std::string>& tokens) {
    CmdConfigProtocolBgpPeerGroup cmd;
    HostInfo hostInfo("testhost");
    return cmd.queryClient(hostInfo, BgpPeerGroupConfig(tokens));
  }

  std::string del(const std::vector<std::string>& tokens) {
    CmdDeleteProtocolBgpPeerGroup cmd;
    HostInfo hostInfo("testhost");
    return cmd.queryClient(hostInfo, BgpPeerGroupRef(tokens));
  }

  const std::vector<bgp::thrift::PeerGroup>& groups() {
    return ConfigSession::getInstance().getBgpConfig().peer_groups().ensure();
  }

  bool sessionFileExists() {
    return std::filesystem::exists(
        ConfigSession::getInstance().getBgpSessionConfigPath());
  }
};

// ==============================================================================
// BgpPeerGroupRef (arg) validation
// ==============================================================================

TEST_F(CmdDeleteBgpPeerGroupTestFixture, argValidation) {
  EXPECT_EQ(BgpPeerGroupRef({"SPINE"}).groupName(), "SPINE");

  // Invalid: empty, empty name, extra tokens.
  EXPECT_THROW(BgpPeerGroupRef({}), std::invalid_argument);
  EXPECT_THROW(BgpPeerGroupRef({""}), std::invalid_argument);
  EXPECT_THROW(BgpPeerGroupRef({"SPINE", "LEAF"}), std::invalid_argument);
}

// ==============================================================================
// queryClient
// ==============================================================================

TEST_F(CmdDeleteBgpPeerGroupTestFixture, deleteExistingGroup) {
  configure({"SPINE", "remote-asn", "65000"});
  configure({"LEAF", "remote-asn", "65001"});
  ASSERT_EQ(groups().size(), 2);

  auto result = del({"SPINE"});
  EXPECT_THAT(result, HasSubstr("Successfully deleted BGP peer-group SPINE"));
  // The other group survives untouched.
  ASSERT_EQ(groups().size(), 1);
  EXPECT_EQ(*groups()[0].name(), "LEAF");
  EXPECT_EQ(groups()[0].remote_as_4_byte().value_or(0), 65001);
  EXPECT_TRUE(sessionFileExists());
}

TEST_F(CmdDeleteBgpPeerGroupTestFixture, deleteUnknownGroupRejected) {
  auto result = del({"NO-SUCH-GROUP"});
  EXPECT_THAT(
      result, HasSubstr("Error: BGP peer-group NO-SUCH-GROUP not found"));
  // Nothing was persisted for the failed delete.
  EXPECT_FALSE(sessionFileExists())
      << "session file should not exist after rejected delete";
}

TEST_F(CmdDeleteBgpPeerGroupTestFixture, deleteUnknownLeavesOthersIntact) {
  configure({"SPINE", "remote-asn", "65000"});
  auto result = del({"LEAF"});
  EXPECT_THAT(result, HasSubstr("not found"));
  ASSERT_EQ(groups().size(), 1);
  EXPECT_EQ(*groups()[0].name(), "SPINE");
}

} // namespace facebook::fboss
