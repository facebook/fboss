// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <configerator/structs/neteng/fboss/bgp/gen-cpp2/bgp_config_types.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

#include "configerator/structs/neteng/fboss/bgp/if/gen-cpp2/bgp_attr_types.h"
#include "fboss/cli/fboss2/commands/config/protocol/bgp/peer-group/CmdConfigProtocolBgpPeerGroup.h"
#include "fboss/cli/fboss2/session/ConfigSession.h"
#include "fboss/cli/fboss2/test/config/CmdConfigTestBase.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"

using namespace ::testing;
using facebook::neteng::fboss::bgp_attr::AddPath;

namespace facebook::fboss {

// The peer-group dispatcher only touches the BGP side of ConfigSession, which
// seeds from thrift schema defaults when neither a staged session nor a
// system bgpcpp.conf exists — so no seed agent config is needed (mirrors
// CmdConfigBgpNeighborTest).
class CmdConfigBgpPeerGroupTestFixture : public CmdConfigTestBase {
 public:
  CmdConfigBgpPeerGroupTestFixture()
      : CmdConfigTestBase("bgp_peer_group_test_%%%%-%%%%-%%%%", "") {}

  void SetUp() override {
    CmdConfigTestBase::SetUp();
    setupTestableConfigSession();
  }

  std::string run(const std::vector<std::string>& tokens) {
    CmdConfigProtocolBgpPeerGroup cmd;
    HostInfo hostInfo("testhost");
    return cmd.queryClient(hostInfo, BgpPeerGroupConfig(tokens));
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
// BgpPeerGroupConfig (arg) validation
// ==============================================================================

TEST_F(CmdConfigBgpPeerGroupTestFixture, argValidation) {
  // Bare create.
  EXPECT_EQ(BgpPeerGroupConfig({"SPINE"}).groupName(), "SPINE");
  EXPECT_TRUE(BgpPeerGroupConfig({"SPINE"}).attr().empty());

  // Single-token attribute.
  auto singleTok = BgpPeerGroupConfig({"SPINE", "remote-asn", "65000"});
  EXPECT_EQ(singleTok.attr(), "remote-asn");
  EXPECT_EQ(singleTok.values(), std::vector<std::string>({"65000"}));

  // Two-token attribute wins longest-prefix match.
  auto twoTok = BgpPeerGroupConfig({"SPINE", "timers", "hold-time", "30"});
  EXPECT_EQ(twoTok.attr(), "timers hold-time");
  EXPECT_EQ(twoTok.values(), std::vector<std::string>({"30"}));

  // Invalid: empty, empty name, unknown attribute, incomplete group.
  EXPECT_THROW(BgpPeerGroupConfig({}), std::invalid_argument);
  EXPECT_THROW(BgpPeerGroupConfig({""}), std::invalid_argument);
  EXPECT_THROW(
      BgpPeerGroupConfig({"SPINE", "no-such-attr", "1"}),
      std::invalid_argument);
  EXPECT_THROW(BgpPeerGroupConfig({"SPINE", "timers"}), std::invalid_argument);
}

// ==============================================================================
// Attribute handlers
// ==============================================================================

TEST_F(CmdConfigBgpPeerGroupTestFixture, bareCreate) {
  auto result = run({"SPINE"});
  EXPECT_THAT(result, HasSubstr("Successfully created BGP peer-group SPINE"));
  ASSERT_EQ(groups().size(), 1);
  EXPECT_EQ(*groups()[0].name(), "SPINE");
  // PeerGroup's only non-optional field is `name`; nothing else is seeded.
  EXPECT_FALSE(groups()[0].local_addr().has_value());
  EXPECT_TRUE(sessionFileExists());

  run({"LEAF"});
  ASSERT_EQ(groups().size(), 2);
  EXPECT_EQ(*groups()[1].name(), "LEAF");
}

TEST_F(CmdConfigBgpPeerGroupTestFixture, setRemoteAsn) {
  auto result = run({"SPINE", "remote-asn", "65000"});
  EXPECT_THAT(result, HasSubstr("65000"));
  ASSERT_EQ(groups().size(), 1);
  EXPECT_EQ(groups()[0].remote_as_4_byte().value_or(0), 65000);
}

TEST_F(CmdConfigBgpPeerGroupTestFixture, timersAccumulate) {
  run({"SPINE", "timers", "hold-time", "90"});
  run({"SPINE", "timers", "keepalive", "30"});
  run({"SPINE", "graceful-restart", "restart-time", "120"});
  ASSERT_EQ(groups().size(), 1);
  const auto& timers = *groups()[0].bgp_peer_timers();
  EXPECT_EQ(*timers.hold_time_seconds(), 90);
  EXPECT_EQ(*timers.keep_alive_seconds(), 30);
  EXPECT_EQ(timers.graceful_restart_seconds().value_or(0), 120);
}

TEST_F(CmdConfigBgpPeerGroupTestFixture, addPathMerge) {
  run({"SPINE", "add-path", "receive", "true"});
  EXPECT_EQ(groups()[0].add_path().value_or(AddPath(0)), AddPath::RECEIVE);

  run({"SPINE", "add-path", "send", "true"});
  EXPECT_EQ(groups()[0].add_path().value_or(AddPath(0)), AddPath::BOTH);

  run({"SPINE", "add-path", "receive", "false"});
  EXPECT_EQ(groups()[0].add_path().value_or(AddPath(0)), AddPath::SEND);

  run({"SPINE", "add-path", "send", "false"});
  EXPECT_FALSE(groups()[0].add_path().has_value());
}

TEST_F(CmdConfigBgpPeerGroupTestFixture, connectMode) {
  run({"SPINE", "connect-mode", "PASSIVE"});
  EXPECT_TRUE(groups()[0].is_passive().value_or(false));

  run({"SPINE", "connect-mode", "ACTIVE"});
  EXPECT_FALSE(groups()[0].is_passive().value_or(true));

  // BOTH has no thrift representation and must be rejected, leaving the
  // previous value intact and nothing new persisted.
  auto result = run({"SPINE", "connect-mode", "BOTH"});
  EXPECT_THAT(result, HasSubstr("Error"));
  EXPECT_FALSE(groups()[0].is_passive().value_or(true));
}

TEST_F(CmdConfigBgpPeerGroupTestFixture, boolAndStringAttributes) {
  run({"SPINE", "rr-client", "true"});
  run({"SPINE", "confed-peer", "false"});
  run({"SPINE", "redistribute-peer", "true"});
  run({"SPINE", "enhanced-route-refresh", "true"});
  run({"SPINE", "afi", "disable-ipv4-afi", "true"});
  run({"SPINE", "afi", "disable-ipv6-afi", "false"});
  run({"SPINE", "afi", "ipv4-over-ipv6-nh", "true"});
  run({"SPINE", "graceful-restart", "stateful-ha", "true"});
  run({"SPINE", "peer-tag", "FSW"});
  run({"SPINE", "ingress-policy", "IN_POLICY"});
  run({"SPINE", "egress-policy", "OUT_POLICY"});
  run({"SPINE", "local-asn", "64512"});
  run({"SPINE", "next-hop-self", "true"});
  run({"SPINE", "description", "spine", "group"});

  ASSERT_EQ(groups().size(), 1);
  const auto& group = groups()[0];
  EXPECT_TRUE(group.is_rr_client().value_or(false));
  EXPECT_FALSE(group.is_confed_peer().value_or(true));
  EXPECT_TRUE(group.is_redistribute_peer().value_or(false));
  EXPECT_TRUE(group.enhanced_route_refresh().value_or(false));
  EXPECT_TRUE(group.disable_ipv4_afi().value_or(false));
  EXPECT_FALSE(group.disable_ipv6_afi().value_or(true));
  EXPECT_TRUE(group.v4_over_v6_nexthop().value_or(false));
  EXPECT_TRUE(group.enable_stateful_ha().value_or(false));
  EXPECT_EQ(group.peer_tag().value_or(""), "FSW");
  EXPECT_EQ(group.ingress_policy_name().value_or(""), "IN_POLICY");
  EXPECT_EQ(group.egress_policy_name().value_or(""), "OUT_POLICY");
  EXPECT_EQ(group.local_as_4_byte().value_or(0), 64512);
  EXPECT_TRUE(group.next_hop_self().value_or(false));
  // Multi-token description is re-joined.
  EXPECT_EQ(group.description().value_or(""), "spine group");
}

TEST_F(CmdConfigBgpPeerGroupTestFixture, routeLimits) {
  run({"SPINE", "max-route", "pre-filter", "45000"});
  run({"SPINE", "max-route", "post-filter", "50000"});
  run({"SPINE", "max-route", "post-warning-threshold", "48000"});
  // Parity attributes carried over from the deleted per-attribute commands.
  run({"SPINE", "max-route", "pre-warning-threshold", "40000"});
  run({"SPINE", "max-route", "pre-warning-only", "true"});
  run({"SPINE", "max-route", "post-warning-only", "false"});
  const auto& group = groups()[0];
  EXPECT_EQ(*group.pre_filter()->max_routes(), 45000);
  EXPECT_EQ(*group.post_filter()->max_routes(), 50000);
  EXPECT_EQ(*group.post_filter()->warning_limit(), 48000);
  EXPECT_EQ(*group.pre_filter()->warning_limit(), 40000);
  EXPECT_TRUE(*group.pre_filter()->warning_only());
  EXPECT_FALSE(*group.post_filter()->warning_only());
}

TEST_F(CmdConfigBgpPeerGroupTestFixture, valueValidation) {
  // ASN must fit in 4 bytes (RFC 6793).
  EXPECT_THAT(run({"SPINE", "remote-asn", "4294967296"}), HasSubstr("Invalid"));
  EXPECT_THAT(
      run({"SPINE", "local-asn", "18446744073709551615"}),
      HasSubstr("Invalid"));
  EXPECT_FALSE(sessionFileExists())
      << "session file should not exist after rejected input";
}

TEST_F(CmdConfigBgpPeerGroupTestFixture, namedGroupsAreDistinct) {
  run({"SPINE", "remote-asn", "65000"});
  run({"LEAF", "remote-asn", "65001"});
  ASSERT_EQ(groups().size(), 2);
  // Re-referencing an existing group by name updates it, not appends.
  run({"SPINE", "local-asn", "64512"});
  EXPECT_EQ(groups().size(), 2);
}

// ==============================================================================
// Reject paths — the error is surfaced and nothing is persisted
// ==============================================================================

TEST_F(CmdConfigBgpPeerGroupTestFixture, invalidValuesRejected) {
  EXPECT_THAT(run({"SPINE", "rr-client", "maybe"}), HasSubstr("Invalid"));
  EXPECT_THAT(
      run({"SPINE", "timers", "hold-time", "-1"}), HasSubstr("non-negative"));
  EXPECT_THAT(
      run({"SPINE", "remote-asn", "not-a-number"}), HasSubstr("Invalid"));
  EXPECT_THAT(
      run({"SPINE", "max-route", "pre-filter", "-5"}),
      HasSubstr("non-negative"));
  EXPECT_FALSE(sessionFileExists())
      << "session file should not exist after rejected input";
}

TEST_F(CmdConfigBgpPeerGroupTestFixture, unsupportedAttributesRejected) {
  // These attributes have no per-peer-group thrift field, so they are not in
  // the dispatch table and are refused at parse time as unknown attributes.
  EXPECT_THROW(run({"SPINE", "peer-port", "1179"}), std::invalid_argument);
  EXPECT_THROW(
      run({"SPINE", "afi", "ipv4-labeled-unicast", "true"}),
      std::invalid_argument);
  EXPECT_THROW(
      run({"SPINE", "afi", "ipv6-labeled-unicast", "true"}),
      std::invalid_argument);
  EXPECT_FALSE(sessionFileExists())
      << "session file should not exist after rejected input";
}

} // namespace facebook::fboss
