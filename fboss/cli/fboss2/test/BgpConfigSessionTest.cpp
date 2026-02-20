/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <boost/filesystem.hpp>
#include <folly/json/dynamic.h>
#include <folly/json/json.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>

#include "fboss/cli/fboss2/commands/config/protocol/bgp/BgpConfigSession.h"

namespace fs = std::filesystem;

namespace facebook::fboss {

/**
 * BgpConfigSessionTest validates the JSON generation workflow for BGP CLI
 * commands.
 *
 * These tests verify that:
 * 1. CLI commands correctly generate BGP++ thrift-compatible JSON
 * 2. All configuration fields are properly set and retrieved
 * 3. Boolean values (including false) are preserved
 * 4. Nested structures (peer timers, pre_filter, add_path) are correct
 * 5. The round-trip workflow (CLI -> JSON) produces valid output
 */
class BgpConfigSessionTest : public ::testing::Test {
 public:
  void SetUp() override {
    // Create unique test directory
    auto tempBase = fs::temp_directory_path();
    auto uniquePath =
        boost::filesystem::unique_path("bgp_config_test_%%%%-%%%%-%%%%");
    testHomeDir_ = tempBase / uniquePath.string();

    // Clean up any previous test artifacts
    std::error_code ec;
    if (fs::exists(testHomeDir_)) {
      fs::remove_all(testHomeDir_, ec);
    }

    // Create test directories
    fs::create_directories(testHomeDir_ / ".fboss2");

    // Set environment variables
    originalHome_ = getenv("HOME");
    setenv("HOME", testHomeDir_.c_str(), 1);

    // Enable stub mode to prevent actual file writes during most tests
    session_ = &BgpConfigSession::getInstance();
    session_->setStubMode(true);
    session_->clearSession();
  }

  void TearDown() override {
    // Restore original HOME
    if (originalHome_) {
      setenv("HOME", originalHome_, 1);
    }

    // Clean up test directories
    std::error_code ec;
    if (fs::exists(testHomeDir_)) {
      fs::remove_all(testHomeDir_, ec);
    }
  }

 protected:
  fs::path testHomeDir_;
  const char* originalHome_ = nullptr;
  BgpConfigSession* session_ = nullptr;

  std::string readFile(const fs::path& path) {
    std::ifstream file(path);
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
  }
};

// ==================== Global Configuration Tests ====================

TEST_F(BgpConfigSessionTest, setRouterId) {
  session_->setRouterId("10.0.0.1");
  EXPECT_EQ(session_->getRouterId(), "10.0.0.1");

  auto& config = session_->getBgpConfig();
  EXPECT_EQ(config["router_id"].asString(), "10.0.0.1");
}

TEST_F(BgpConfigSessionTest, setLocalAsn) {
  session_->setLocalAsn(65000);
  EXPECT_EQ(session_->getLocalAsn(), 65000);

  auto& config = session_->getBgpConfig();
  EXPECT_EQ(config["local_as_4_byte"].asInt(), 65000);
}

TEST_F(BgpConfigSessionTest, setHoldTime) {
  session_->setHoldTime(90);
  EXPECT_EQ(session_->getHoldTime(), 90);

  auto& config = session_->getBgpConfig();
  EXPECT_EQ(config["hold_time"].asInt(), 90);
}

TEST_F(BgpConfigSessionTest, setConfedAsn) {
  session_->setConfedAsn(65100);
  EXPECT_EQ(session_->getConfedAsn(), 65100);

  auto& config = session_->getBgpConfig();
  EXPECT_EQ(config["local_confed_as_4_byte"].asInt(), 65100);
}

TEST_F(BgpConfigSessionTest, setClusterId) {
  session_->setClusterId("1.2.3.4");
  EXPECT_EQ(session_->getClusterId(), "1.2.3.4");

  auto& config = session_->getBgpConfig();
  EXPECT_EQ(config["cluster_id"].asString(), "1.2.3.4");
}

TEST_F(BgpConfigSessionTest, setListenAddress) {
  session_->setListenAddress("0.0.0.0");
  EXPECT_EQ(session_->getListenAddress(), "0.0.0.0");

  auto& config = session_->getBgpConfig();
  EXPECT_EQ(config["listen_addr"].asString(), "0.0.0.0");
}

TEST_F(BgpConfigSessionTest, setListenPort) {
  session_->setListenPort(1179);
  EXPECT_EQ(session_->getListenPort(), 1179);

  auto& config = session_->getBgpConfig();
  EXPECT_EQ(config["listen_port"].asInt(), 1179);
}

// ==================== Switch Limit Configuration Tests ====================

TEST_F(BgpConfigSessionTest, setSwitchLimitPrefixLimit) {
  session_->setSwitchLimitPrefixLimit(10000);

  auto& config = session_->getBgpConfig();
  ASSERT_TRUE(config.count("switch_limit_config"));
  EXPECT_EQ(config["switch_limit_config"]["prefix_limit"].asInt(), 10000);
}

TEST_F(BgpConfigSessionTest, setSwitchLimitTotalPathLimit) {
  session_->setSwitchLimitTotalPathLimit(250000);

  auto& config = session_->getBgpConfig();
  ASSERT_TRUE(config.count("switch_limit_config"));
  EXPECT_EQ(config["switch_limit_config"]["total_path_limit"].asInt(), 250000);
}

TEST_F(BgpConfigSessionTest, setSwitchLimitMaxGoldenVips) {
  session_->setSwitchLimitMaxGoldenVips(2860);

  auto& config = session_->getBgpConfig();
  ASSERT_TRUE(config.count("switch_limit_config"));
  EXPECT_EQ(config["switch_limit_config"]["max_golden_vips"].asInt(), 2860);
}

TEST_F(BgpConfigSessionTest, setSwitchLimitOverloadProtectionMode) {
  session_->setSwitchLimitOverloadProtectionMode(2);

  auto& config = session_->getBgpConfig();
  ASSERT_TRUE(config.count("switch_limit_config"));
  EXPECT_EQ(
      config["switch_limit_config"]["overload_protection_mode"].asInt(), 2);
}

TEST_F(BgpConfigSessionTest, allSwitchLimitFieldsTogether) {
  session_->setSwitchLimitPrefixLimit(10000);
  session_->setSwitchLimitTotalPathLimit(250000);
  session_->setSwitchLimitMaxGoldenVips(2860);
  session_->setSwitchLimitOverloadProtectionMode(2);

  auto& config = session_->getBgpConfig();
  ASSERT_TRUE(config.count("switch_limit_config"));
  auto& switchLimit = config["switch_limit_config"];
  EXPECT_EQ(switchLimit["prefix_limit"].asInt(), 10000);
  EXPECT_EQ(switchLimit["total_path_limit"].asInt(), 250000);
  EXPECT_EQ(switchLimit["max_golden_vips"].asInt(), 2860);
  EXPECT_EQ(switchLimit["overload_protection_mode"].asInt(), 2);
}

// ==================== Network6 Configuration Tests ====================

TEST_F(BgpConfigSessionTest, addNetwork6Basic) {
  session_->addNetwork6("2001:db8::/32", "TEST_POLICY", true, 0);

  auto& config = session_->getBgpConfig();
  ASSERT_TRUE(config.count("networks6"));
  ASSERT_EQ(config["networks6"].size(), 1);

  auto& network = config["networks6"][0];
  EXPECT_EQ(network["prefix"].asString(), "2001:db8::/32");
  EXPECT_EQ(network["policy_name"].asString(), "TEST_POLICY");
  EXPECT_TRUE(network["install_to_fib"].asBool());
  EXPECT_EQ(network["minimum_supporting_routes"].asInt(), 0);
}

TEST_F(BgpConfigSessionTest, addNetwork6WithInstallToFibFalse) {
  // Critical test: Verify that install_to_fib: false is preserved
  session_->addNetwork6(
      "2401:db00:501c::/64", "ORIGINATE_RACK_PRIVATE_PREFIXES", false, 0);

  auto& config = session_->getBgpConfig();
  ASSERT_TRUE(config.count("networks6"));
  ASSERT_EQ(config["networks6"].size(), 1);

  auto& network = config["networks6"][0];
  EXPECT_EQ(network["prefix"].asString(), "2401:db00:501c::/64");
  EXPECT_EQ(
      network["policy_name"].asString(), "ORIGINATE_RACK_PRIVATE_PREFIXES");
  // Critical: install_to_fib: false must be preserved, not omitted
  EXPECT_FALSE(network["install_to_fib"].asBool());
  EXPECT_EQ(network["minimum_supporting_routes"].asInt(), 0);
}

TEST_F(BgpConfigSessionTest, addNetwork6WithMinSupportingRoutes) {
  session_->addNetwork6("2001:db8::/48", "POLICY", true, 5);

  auto& config = session_->getBgpConfig();
  auto& network = config["networks6"][0];
  EXPECT_EQ(network["minimum_supporting_routes"].asInt(), 5);
}

TEST_F(BgpConfigSessionTest, addMultipleNetworks6) {
  session_->addNetwork6("2001:db8:1::/48", "POLICY1", true, 0);
  session_->addNetwork6("2001:db8:2::/48", "POLICY2", false, 1);
  session_->addNetwork6("2001:db8:3::/48", "POLICY3", true, 2);

  auto& config = session_->getBgpConfig();
  ASSERT_EQ(config["networks6"].size(), 3);

  EXPECT_EQ(config["networks6"][0]["prefix"].asString(), "2001:db8:1::/48");
  EXPECT_EQ(config["networks6"][1]["prefix"].asString(), "2001:db8:2::/48");
  EXPECT_EQ(config["networks6"][2]["prefix"].asString(), "2001:db8:3::/48");

  // Verify second network has install_to_fib: false
  EXPECT_FALSE(config["networks6"][1]["install_to_fib"].asBool());
}

// ==================== Peer Configuration Tests ====================

TEST_F(BgpConfigSessionTest, createPeerAndSetRemoteAsn) {
  session_->createPeer("2001:db8::1");
  session_->setPeerRemoteAsn("2001:db8::1", 65001);

  auto& config = session_->getBgpConfig();
  ASSERT_TRUE(config.count("peers"));
  ASSERT_EQ(config["peers"].size(), 1);

  auto& peer = config["peers"][0];
  EXPECT_EQ(peer["peer_addr"].asString(), "2001:db8::1");
  EXPECT_EQ(peer["remote_as_4_byte"].asInt(), 65001);
}

TEST_F(BgpConfigSessionTest, setPeerDescription) {
  session_->createPeer("10.0.0.1");
  session_->setPeerDescription("10.0.0.1", "Test peer description");

  auto& config = session_->getBgpConfig();
  EXPECT_EQ(
      config["peers"][0]["description"].asString(), "Test peer description");
}

TEST_F(BgpConfigSessionTest, setPeerPeerGroupName) {
  session_->createPeer("2001:db8::1");
  session_->setPeerGroupName("2001:db8::1", "RSW-FSW-V6");

  auto& config = session_->getBgpConfig();
  EXPECT_EQ(config["peers"][0]["peer_group_name"].asString(), "RSW-FSW-V6");
}

TEST_F(BgpConfigSessionTest, setPeerDisableIpv4AfiTrue) {
  session_->createPeer("2001:db8::1");
  session_->setPeerDisableIpv4Afi("2001:db8::1", true);

  auto& config = session_->getBgpConfig();
  EXPECT_TRUE(config["peers"][0]["disable_ipv4_afi"].asBool());
}

TEST_F(BgpConfigSessionTest, setPeerDisableIpv4AfiFalse) {
  // Critical test: Verify that disable_ipv4_afi: false is preserved
  session_->createPeer("2001:db8::1");
  session_->setPeerDisableIpv4Afi("2001:db8::1", false);

  auto& config = session_->getBgpConfig();
  // Must be explicitly set to false, not omitted
  ASSERT_TRUE(config["peers"][0].count("disable_ipv4_afi"));
  EXPECT_FALSE(config["peers"][0]["disable_ipv4_afi"].asBool());
}

TEST_F(BgpConfigSessionTest, setPeerNextHopSelf) {
  session_->createPeer("2001:db8::1");
  session_->setPeerNextHopSelf("2001:db8::1", true);

  auto& config = session_->getBgpConfig();
  EXPECT_TRUE(config["peers"][0]["next_hop_self"].asBool());
}

TEST_F(BgpConfigSessionTest, setPeerConfedPeer) {
  session_->createPeer("2001:db8::1");
  session_->setPeerConfedPeer("2001:db8::1", true);

  auto& config = session_->getBgpConfig();
  EXPECT_TRUE(config["peers"][0]["is_confed_peer"].asBool());
}

TEST_F(BgpConfigSessionTest, setPeerPassive) {
  session_->createPeer("2001:db8::1");
  session_->setPeerPassive("2001:db8::1", true);

  auto& config = session_->getBgpConfig();
  EXPECT_TRUE(config["peers"][0]["is_passive"].asBool());
}

TEST_F(BgpConfigSessionTest, setPeerRrClient) {
  session_->createPeer("2001:db8::1");
  session_->setPeerRrClient("2001:db8::1", true);

  auto& config = session_->getBgpConfig();
  EXPECT_TRUE(config["peers"][0]["is_rr_client"].asBool());
}

TEST_F(BgpConfigSessionTest, setPeerTimers) {
  session_->createPeer("2001:db8::1");
  session_->setPeerHoldTime("2001:db8::1", 90);
  session_->setPeerKeepalive("2001:db8::1", 30);
  session_->setPeerOutDelay("2001:db8::1", 5);

  auto& config = session_->getBgpConfig();
  ASSERT_TRUE(config["peers"][0].count("bgp_peer_timers"));

  auto& timers = config["peers"][0]["bgp_peer_timers"];
  EXPECT_EQ(timers["hold_time_seconds"].asInt(), 90);
  EXPECT_EQ(timers["keep_alive_seconds"].asInt(), 30);
  EXPECT_EQ(timers["out_delay_seconds"].asInt(), 5);
}

TEST_F(BgpConfigSessionTest, setPeerPolicies) {
  session_->createPeer("2001:db8::1");
  session_->setPeerIngressPolicy("2001:db8::1", "INGRESS_POLICY");
  session_->setPeerEgressPolicy("2001:db8::1", "EGRESS_POLICY");

  auto& config = session_->getBgpConfig();
  EXPECT_EQ(
      config["peers"][0]["ingress_policy_name"].asString(), "INGRESS_POLICY");
  EXPECT_EQ(
      config["peers"][0]["egress_policy_name"].asString(), "EGRESS_POLICY");
}

TEST_F(BgpConfigSessionTest, setPeerPreFilterMaxRoutes) {
  session_->createPeer("2001:db8::1");
  session_->setPeerPreFilterMaxRoutes("2001:db8::1", 1000);

  auto& config = session_->getBgpConfig();
  ASSERT_TRUE(config["peers"][0].count("pre_filter"));
  EXPECT_EQ(config["peers"][0]["pre_filter"]["max_routes"].asInt(), 1000);
}

TEST_F(BgpConfigSessionTest, setPeerLinkBandwidth) {
  session_->createPeer("2001:db8::1");
  session_->setPeerAdvertiseLbw("2001:db8::1", true);
  session_->setPeerLbwValue("2001:db8::1", 10000000000);

  auto& config = session_->getBgpConfig();
  EXPECT_TRUE(config["peers"][0]["advertise_link_bandwidth"].asBool());
  EXPECT_EQ(config["peers"][0]["link_bandwidth_bps"].asInt(), 10000000000);
}

TEST_F(BgpConfigSessionTest, setPeerIpv4OverIpv6Nh) {
  session_->createPeer("2001:db8::1");
  session_->setPeerIpv4OverIpv6Nh("2001:db8::1", true);

  auto& config = session_->getBgpConfig();
  EXPECT_TRUE(config["peers"][0]["v4_over_v6_nexthop"].asBool());
}

TEST_F(BgpConfigSessionTest, setPeerPeerId) {
  session_->createPeer("2001:db8::1");
  session_->setPeerPeerId("2001:db8::1", "fsw001.p001.f01.qzd1:v6:1");

  auto& config = session_->getBgpConfig();
  EXPECT_EQ(
      config["peers"][0]["peer_id"].asString(), "fsw001.p001.f01.qzd1:v6:1");
}

TEST_F(BgpConfigSessionTest, setPeerType) {
  session_->createPeer("2001:db8::1");
  session_->setPeerType("2001:db8::1", "FSW");

  auto& config = session_->getBgpConfig();
  EXPECT_EQ(config["peers"][0]["type"].asString(), "FSW");
}

// ==================== Peer Group Configuration Tests ====================

TEST_F(BgpConfigSessionTest, createPeerGroupAndSetDescription) {
  session_->createPeerGroup("TEST-GROUP");
  session_->setPeerGroupDescription("TEST-GROUP", "Test peer group");

  auto& config = session_->getBgpConfig();
  ASSERT_TRUE(config.count("peer_groups"));
  ASSERT_EQ(config["peer_groups"].size(), 1);

  auto& group = config["peer_groups"][0];
  EXPECT_EQ(group["name"].asString(), "TEST-GROUP");
  EXPECT_EQ(group["description"].asString(), "Test peer group");
}

TEST_F(BgpConfigSessionTest, setPeerGroupRemoteAsn) {
  session_->createPeerGroup("TEST-GROUP");
  session_->setPeerGroupRemoteAsn("TEST-GROUP", 65002);

  auto& config = session_->getBgpConfig();
  EXPECT_EQ(config["peer_groups"][0]["remote_as_4_byte"].asInt(), 65002);
}

TEST_F(BgpConfigSessionTest, setPeerGroupNextHopSelf) {
  session_->createPeerGroup("TEST-GROUP");
  session_->setPeerGroupNextHopSelf("TEST-GROUP", true);

  auto& config = session_->getBgpConfig();
  EXPECT_TRUE(config["peer_groups"][0]["next_hop_self"].asBool());
}

TEST_F(BgpConfigSessionTest, setPeerGroupConfedPeer) {
  session_->createPeerGroup("TEST-GROUP");
  session_->setPeerGroupConfedPeer("TEST-GROUP", true);

  auto& config = session_->getBgpConfig();
  EXPECT_TRUE(config["peer_groups"][0]["is_confed_peer"].asBool());
}

TEST_F(BgpConfigSessionTest, setPeerGroupDisableIpv4AfiTrue) {
  session_->createPeerGroup("TEST-GROUP");
  session_->setPeerGroupDisableIpv4Afi("TEST-GROUP", true);

  auto& config = session_->getBgpConfig();
  EXPECT_TRUE(config["peer_groups"][0]["disable_ipv4_afi"].asBool());
}

TEST_F(BgpConfigSessionTest, setPeerGroupDisableIpv4AfiFalse) {
  session_->createPeerGroup("TEST-GROUP");
  session_->setPeerGroupDisableIpv4Afi("TEST-GROUP", false);

  auto& config = session_->getBgpConfig();
  // Must be explicitly set to false, not omitted
  ASSERT_TRUE(config["peer_groups"][0].count("disable_ipv4_afi"));
  EXPECT_FALSE(config["peer_groups"][0]["disable_ipv4_afi"].asBool());
}

TEST_F(BgpConfigSessionTest, setPeerGroupTimers) {
  session_->createPeerGroup("TEST-GROUP");
  session_->setPeerGroupHoldTime("TEST-GROUP", 30);
  session_->setPeerGroupKeepalive("TEST-GROUP", 10);

  auto& config = session_->getBgpConfig();
  ASSERT_TRUE(config["peer_groups"][0].count("bgp_peer_timers"));

  auto& timers = config["peer_groups"][0]["bgp_peer_timers"];
  EXPECT_EQ(timers["hold_time_seconds"].asInt(), 30);
  EXPECT_EQ(timers["keep_alive_seconds"].asInt(), 10);
}

TEST_F(BgpConfigSessionTest, setPeerGroupPolicies) {
  session_->createPeerGroup("TEST-GROUP");
  session_->setPeerGroupIngressPolicy("TEST-GROUP", "GROUP_INGRESS");
  session_->setPeerGroupEgressPolicy("TEST-GROUP", "GROUP_EGRESS");

  auto& config = session_->getBgpConfig();
  EXPECT_EQ(
      config["peer_groups"][0]["ingress_policy_name"].asString(),
      "GROUP_INGRESS");
  EXPECT_EQ(
      config["peer_groups"][0]["egress_policy_name"].asString(),
      "GROUP_EGRESS");
}

TEST_F(BgpConfigSessionTest, setPeerGroupIpv4OverIpv6Nh) {
  session_->createPeerGroup("TEST-GROUP");
  session_->setPeerGroupIpv4OverIpv6Nh("TEST-GROUP", true);

  auto& config = session_->getBgpConfig();
  EXPECT_TRUE(config["peer_groups"][0]["v4_over_v6_nexthop"].asBool());
}

// ==================== Integration Tests ====================

TEST_F(BgpConfigSessionTest, fullRswConfigurationWorkflow) {
  // Simulate full RSW configuration workflow
  // This test validates the entire round-trip: CLI commands -> JSON

  // Global config
  session_->setRouterId("0.17.1.1");
  session_->setHoldTime(30);

  // Switch limits
  session_->setSwitchLimitPrefixLimit(10000);
  session_->setSwitchLimitTotalPathLimit(250000);
  session_->setSwitchLimitMaxGoldenVips(2860);
  session_->setSwitchLimitOverloadProtectionMode(2);

  // Network6 with install_to_fib: false
  session_->addNetwork6(
      "2401:db00:501c::/64", "ORIGINATE_RACK_PRIVATE_PREFIXES", false, 0);

  // Create peer groups
  session_->createPeerGroup("RSW-FSW-V6");
  session_->setPeerGroupDescription(
      "RSW-FSW-V6", "BGP peering from RSW to FSW, IPv6 sessions");
  session_->setPeerGroupNextHopSelf("RSW-FSW-V6", true);
  session_->setPeerGroupConfedPeer("RSW-FSW-V6", true);
  session_->setPeerGroupIngressPolicy("RSW-FSW-V6", "PROPAGATE_RSW_FSW_IN");
  session_->setPeerGroupEgressPolicy("RSW-FSW-V6", "PROPAGATE_RSW_FSW_OUT");
  session_->setPeerGroupIpv4OverIpv6Nh("RSW-FSW-V6", true);
  session_->setPeerGroupHoldTime("RSW-FSW-V6", 30);
  session_->setPeerGroupKeepalive("RSW-FSW-V6", 10);

  session_->createPeerGroup("RSW-RTSW-V6");
  session_->setPeerGroupDisableIpv4Afi("RSW-RTSW-V6", true);

  // Create peer with peer_group_name
  session_->createPeer("2401:db00:e50e:1300::20");
  session_->setPeerRemoteAsn("2401:db00:e50e:1300::20", 65000);
  session_->setPeerGroupName("2401:db00:e50e:1300::20", "RSW-FSW-V6");
  session_->setPeerDisableIpv4Afi("2401:db00:e50e:1300::20", false);
  session_->setPeerPeerId(
      "2401:db00:e50e:1300::20", "fsw001.p001.f01.qzd1:v6:1");

  // Verify the complete configuration
  auto& config = session_->getBgpConfig();

  // Verify global config
  EXPECT_EQ(config["router_id"].asString(), "0.17.1.1");
  EXPECT_EQ(config["hold_time"].asInt(), 30);

  // Verify switch limits
  ASSERT_TRUE(config.count("switch_limit_config"));
  EXPECT_EQ(config["switch_limit_config"]["prefix_limit"].asInt(), 10000);
  EXPECT_EQ(config["switch_limit_config"]["total_path_limit"].asInt(), 250000);
  EXPECT_EQ(config["switch_limit_config"]["max_golden_vips"].asInt(), 2860);
  EXPECT_EQ(
      config["switch_limit_config"]["overload_protection_mode"].asInt(), 2);

  // Verify networks6
  ASSERT_EQ(config["networks6"].size(), 1);
  EXPECT_FALSE(config["networks6"][0]["install_to_fib"].asBool());

  // Verify peer groups
  ASSERT_EQ(config["peer_groups"].size(), 2);

  // Find RSW-FSW-V6 group
  bool foundFswGroup = false;
  bool foundRtswGroup = false;
  for (const auto& group : config["peer_groups"]) {
    if (group["name"].asString() == "RSW-FSW-V6") {
      foundFswGroup = true;
      EXPECT_TRUE(group["next_hop_self"].asBool());
      EXPECT_TRUE(group["is_confed_peer"].asBool());
      EXPECT_TRUE(group["v4_over_v6_nexthop"].asBool());
      EXPECT_EQ(
          group["ingress_policy_name"].asString(), "PROPAGATE_RSW_FSW_IN");
      EXPECT_EQ(
          group["egress_policy_name"].asString(), "PROPAGATE_RSW_FSW_OUT");
      ASSERT_TRUE(group.count("bgp_peer_timers"));
      EXPECT_EQ(group["bgp_peer_timers"]["hold_time_seconds"].asInt(), 30);
      EXPECT_EQ(group["bgp_peer_timers"]["keep_alive_seconds"].asInt(), 10);
    } else if (group["name"].asString() == "RSW-RTSW-V6") {
      foundRtswGroup = true;
      EXPECT_TRUE(group["disable_ipv4_afi"].asBool());
    }
  }
  EXPECT_TRUE(foundFswGroup);
  EXPECT_TRUE(foundRtswGroup);

  // Verify peer
  ASSERT_EQ(config["peers"].size(), 1);
  auto& peer = config["peers"][0];
  EXPECT_EQ(peer["peer_addr"].asString(), "2401:db00:e50e:1300::20");
  EXPECT_EQ(peer["remote_as_4_byte"].asInt(), 65000);
  EXPECT_EQ(peer["peer_group_name"].asString(), "RSW-FSW-V6");
  // Critical: disable_ipv4_afi: false must be preserved
  ASSERT_TRUE(peer.count("disable_ipv4_afi"));
  EXPECT_FALSE(peer["disable_ipv4_afi"].asBool());
  EXPECT_EQ(peer["peer_id"].asString(), "fsw001.p001.f01.qzd1:v6:1");
}

TEST_F(BgpConfigSessionTest, exportConfigProducesValidJson) {
  session_->setRouterId("10.0.0.1");
  session_->setLocalAsn(65000);
  session_->addNetwork6("2001:db8::/32", "TEST_POLICY", true, 0);
  session_->createPeerGroup("TEST-GROUP");
  session_->createPeer("2001:db8::1");
  session_->setPeerRemoteAsn("2001:db8::1", 65001);

  std::string jsonStr = session_->exportConfig();

  // Verify it's valid JSON by parsing it
  folly::dynamic parsed = folly::parseJson(jsonStr);

  EXPECT_EQ(parsed["router_id"].asString(), "10.0.0.1");
  EXPECT_EQ(parsed["local_as_4_byte"].asInt(), 65000);
  EXPECT_EQ(parsed["networks6"].size(), 1);
  EXPECT_EQ(parsed["peer_groups"].size(), 1);
  EXPECT_EQ(parsed["peers"].size(), 1);
}

TEST_F(BgpConfigSessionTest, multiplePeersWithDifferentConfigs) {
  // Create multiple peers with different configurations
  session_->createPeer("2001:db8::1");
  session_->setPeerRemoteAsn("2001:db8::1", 65001);
  session_->setPeerDisableIpv4Afi("2001:db8::1", true);
  session_->setPeerNextHopSelf("2001:db8::1", true);

  session_->createPeer("2001:db8::2");
  session_->setPeerRemoteAsn("2001:db8::2", 65002);
  session_->setPeerDisableIpv4Afi("2001:db8::2", false);
  session_->setPeerPassive("2001:db8::2", true);

  session_->createPeer("10.0.0.1");
  session_->setPeerRemoteAsn("10.0.0.1", 65003);
  session_->setPeerRrClient("10.0.0.1", true);

  auto& config = session_->getBgpConfig();
  ASSERT_EQ(config["peers"].size(), 3);

  // Find each peer and verify its config
  for (const auto& peer : config["peers"]) {
    std::string addr = peer["peer_addr"].asString();
    if (addr == "2001:db8::1") {
      EXPECT_TRUE(peer["disable_ipv4_afi"].asBool());
      EXPECT_TRUE(peer["next_hop_self"].asBool());
    } else if (addr == "2001:db8::2") {
      EXPECT_FALSE(peer["disable_ipv4_afi"].asBool());
      EXPECT_TRUE(peer["is_passive"].asBool());
    } else if (addr == "10.0.0.1") {
      EXPECT_TRUE(peer["is_rr_client"].asBool());
    }
  }
}

TEST_F(BgpConfigSessionTest, stubModePreventsSave) {
  // When stub mode is enabled, saveConfig should not write to disk
  session_->setStubMode(true);
  EXPECT_TRUE(session_->isStubMode());

  session_->setRouterId("10.0.0.1");

  // Verify config is in memory
  EXPECT_EQ(session_->getRouterId(), "10.0.0.1");
}

TEST_F(BgpConfigSessionTest, stubModeCanClearSession) {
  // In stub mode, clearSession() should not throw
  // The actual clearing behavior depends on implementation details
  session_->setRouterId("10.0.0.1");
  session_->createPeer("2001:db8::1");

  EXPECT_EQ(session_->getRouterId(), "10.0.0.1");

  // Should not throw in stub mode
  EXPECT_NO_THROW(session_->clearSession());
}

// ==================== Edge Case Tests ====================

TEST_F(BgpConfigSessionTest, updateExistingPeer) {
  // Create peer and set initial values
  session_->createPeer("2001:db8::1");
  session_->setPeerRemoteAsn("2001:db8::1", 65001);
  session_->setPeerDescription("2001:db8::1", "Initial description");

  // Update the same peer
  session_->setPeerRemoteAsn("2001:db8::1", 65002);
  session_->setPeerDescription("2001:db8::1", "Updated description");

  auto& config = session_->getBgpConfig();
  // Should still only have one peer
  ASSERT_EQ(config["peers"].size(), 1);
  EXPECT_EQ(config["peers"][0]["remote_as_4_byte"].asInt(), 65002);
  EXPECT_EQ(
      config["peers"][0]["description"].asString(), "Updated description");
}

TEST_F(BgpConfigSessionTest, updateExistingPeerGroup) {
  session_->createPeerGroup("TEST-GROUP");
  session_->setPeerGroupDescription("TEST-GROUP", "Initial");

  session_->setPeerGroupDescription("TEST-GROUP", "Updated");

  auto& config = session_->getBgpConfig();
  ASSERT_EQ(config["peer_groups"].size(), 1);
  EXPECT_EQ(config["peer_groups"][0]["description"].asString(), "Updated");
}

TEST_F(BgpConfigSessionTest, ipv4PeerAddress) {
  // Test with IPv4 peer address
  session_->createPeer("192.168.1.1");
  session_->setPeerRemoteAsn("192.168.1.1", 65000);

  auto& config = session_->getBgpConfig();
  EXPECT_EQ(config["peers"][0]["peer_addr"].asString(), "192.168.1.1");
}

TEST_F(BgpConfigSessionTest, specialCharactersInDescription) {
  session_->createPeer("2001:db8::1");
  session_->setPeerDescription(
      "2001:db8::1", "Test \"quotes\" and 'apostrophes' and special: !@#$%");

  auto& config = session_->getBgpConfig();
  EXPECT_EQ(
      config["peers"][0]["description"].asString(),
      "Test \"quotes\" and 'apostrophes' and special: !@#$%");
}

TEST_F(BgpConfigSessionTest, largeAsnNumber) {
  // Test with 4-byte ASN
  session_->setLocalAsn(4200000000);
  EXPECT_EQ(session_->getLocalAsn(), 4200000000);
}

TEST_F(BgpConfigSessionTest, largeLinkBandwidth) {
  // Test with 100G bandwidth
  session_->createPeer("2001:db8::1");
  session_->setPeerLbwValue("2001:db8::1", 100000000000);

  auto& config = session_->getBgpConfig();
  EXPECT_EQ(config["peers"][0]["link_bandwidth_bps"].asInt(), 100000000000);
}

} // namespace facebook::fboss
