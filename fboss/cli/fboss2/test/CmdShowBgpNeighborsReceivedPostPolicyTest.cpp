// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <folly/json.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <thrift/lib/cpp2/reflection/testing.h> // NOLINT(misc-include-cleaner)
#include <sstream>
#include <vector>
#include "fboss/cli/fboss2/test/CmdHandlerTestBase.h"

#include "configerator/structs/neteng/fboss/bgp/if/gen-cpp2/bgp_attr_types.h"
#include "fboss/agent/AddressUtil.h" // NOLINT(misc-include-cleaner)
#include "fboss/bgp/if/gen-cpp2/bgp_thrift_types.h"
#include "fboss/cli/fboss2/commands/show/bgp/neighbors/received/BgpNeighborsReceivedPostPolicy.h"
#include "fboss/cli/fboss2/test/CmdBgpTestUtils.h"
#include "folly/IPAddress.h" // NOLINT(misc-include-cleaner)
#ifndef IS_OSS
#include "nettools/common/TestUtils.h"
#endif

using namespace ::testing;
using facebook::neteng::fboss::bgp::thrift::TBgpPath;
using facebook::neteng::fboss::bgp_attr::TAsPath;
using facebook::neteng::fboss::bgp_attr::TIpPrefix;
namespace facebook::fboss {

class NeighborsReceivedPostPolicyTestFixture : public CmdHandlerTestBase {
 public:
  std::map<TIpPrefix, std::vector<TBgpPath>> receivedNetworks_;
  std::string lookableIp_;
  void SetUp() override {
    CmdHandlerTestBase::SetUp();
    receivedNetworks_ = getReceivedNetworks(
        "8.0.0.0/32", // prefixAddress
        "8.0.0.1"); // nextHopAddress
    lookableIp_ = "1.2.3.4";
  }
};

class NeighborsReceivedPostPolicyTestFixtureWithoutMed
    : public CmdHandlerTestBase {
 public:
  std::map<TIpPrefix, std::vector<TBgpPath>> receivedNetworks_;
  void SetUp() override {
    CmdHandlerTestBase::SetUp();
    receivedNetworks_ = getReceivedNetworks(
        "8.0.0.0/32", // prefixAddress
        "8.0.0.1", // nextHopAddress
        true, // setCommunity
        true, // setAsPath
        false, // setExtCommunity
        std::nullopt, // clusterList
        std::nullopt, // originatorId
        true, // setPolicy
        false); // setMed
  }
};

TEST_F(NeighborsReceivedPostPolicyTestFixture, queryClient) {
  setupMockedBgpServer();
  EXPECT_CALL(getMockBgp(), getPostfilterReceivedNetworks2(_, _))
      .WillOnce(Invoke([&](std::map<TIpPrefix, std::vector<TBgpPath>>& networks,
                           std::unique_ptr<std::string> queriedIp) {
        networks = receivedNetworks_;
        queriedIp = std::make_unique<std::string>(lookableIp_);
      }));
  auto results = BgpNeighborsReceivedPostPolicy().queryClient(
      localhost(), {lookableIp_}, {});
  ASSERT_EQ(results.networkPath()->size(), receivedNetworks_.size());
  for (const auto& [prefix, paths] : receivedNetworks_) {
    ASSERT_TRUE(results.networkPath()->count(prefix));
    EXPECT_THRIFT_EQ_VECTOR(results.networkPath()->at(prefix), paths);
  }
}

TEST_F(NeighborsReceivedPostPolicyTestFixture, printOutput) {
  setupMockedBgpServer();
  EXPECT_CALL(getMockBgp(), getRunningConfig(_))
      .WillRepeatedly(Invoke([&](std::string& config) {
        // clang-format off
        folly::dynamic value = folly::dynamic::object
          ("communities",
          folly::dynamic::array(
          folly::dynamic::object("name", "FABRIC_POD_RSW_LOOP")
          ("description", "rsw loopback")
          ("communities", folly::dynamic::array("65527:12705"))
          )
        )
        ("localprefs",
        folly::dynamic::array(
          folly::dynamic::object("localpref", 20)
          ("name", "LOCALPREF_CTRL_BACKUP")
          ("description", "low-priority supplementary/backup routes from bgp controller"),
          folly::dynamic::object("localpref", 25)
          ("name", "LOCALPREF_DEPRIO")
          ("description", "deprioritized local preference value"))
        );
        // clang-format on
        config = folly::toPrettyJson(value);
      }));
  std::stringstream ss;
  NetworkPathWithHost networkPathWithHost;
  networkPathWithHost.networkPath() = receivedNetworks_;
  networkPathWithHost.host() = localhost().getName();
  networkPathWithHost.oobName() = localhost().getOobName();
  networkPathWithHost.ip() = localhost().getIpStr();
  BgpNeighborsReceivedPostPolicy().printOutput(networkPathWithHost, ss);
  std::string output = ss.str();

  std::string expectedOutput =
      "---\n"
      "Network: 8.0.0.0/32\n"
      "Nexthop: 8.0.0.1\n"
      "Router/OriginatorId:   --  \n"
      "ClusterList: []\n"
      "Communities: FABRIC_POD_RSW_LOOP/65527:12705\n"
      "ExtCommunities: \n"
      "AsPath: 65301\n"
      "LocalPref: DEPRIO/25\n"
      "Origin: INCOMPLETE\n"
      "MED: 10\n"
      "LastModified: 2021-10-26 13:07:40.724 PDT\n"
      "Policy: Accepted/Modified by PROPAGATE_RSW_FSW_IN term N/A\n";
  EXPECT_EQ(output, expectedOutput);
}

TEST_F(NeighborsReceivedPostPolicyTestFixtureWithoutMed, printOutput) {
  setupMockedBgpServer();
  EXPECT_CALL(getMockBgp(), getRunningConfig(_))
      .WillRepeatedly(Invoke([&](std::string& config) {
        // clang-format off
        folly::dynamic value = folly::dynamic::object
          ("communities",
          folly::dynamic::array(
          folly::dynamic::object("name", "FABRIC_POD_RSW_LOOP")
          ("description", "rsw loopback")
          ("communities", folly::dynamic::array("65527:12705"))
          )
        )
        ("localprefs",
        folly::dynamic::array(
          folly::dynamic::object("localpref", 20)
          ("name", "LOCALPREF_CTRL_BACKUP")
          ("description", "low-priority supplementary/backup routes from bgp controller"),
          folly::dynamic::object("localpref", 25)
          ("name", "LOCALPREF_DEPRIO")
          ("description", "deprioritized local preference value"))
        );
        // clang-format on
        config = folly::toPrettyJson(value);
      }));
  std::stringstream ss;
  NetworkPathWithHost networkPathWithHost;
  networkPathWithHost.networkPath() = receivedNetworks_;
  networkPathWithHost.host() = localhost().getName();
  networkPathWithHost.oobName() = localhost().getOobName();
  networkPathWithHost.ip() = localhost().getIpStr();
  BgpNeighborsReceivedPostPolicy().printOutput(networkPathWithHost, ss);
  std::string output = ss.str();

  std::string expectedOutput =
      "---\n"
      "Network: 8.0.0.0/32\n"
      "Nexthop: 8.0.0.1\n"
      "Router/OriginatorId:   --  \n"
      "ClusterList: []\n"
      "Communities: FABRIC_POD_RSW_LOOP/65527:12705\n"
      "ExtCommunities: \n"
      "AsPath: 65301\n"
      "LocalPref: DEPRIO/25\n"
      "Origin: INCOMPLETE\n"
      "MED: Not set\n"
      "LastModified: 2021-10-26 13:07:40.724 PDT\n"
      "Policy: Accepted/Modified by PROPAGATE_RSW_FSW_IN term N/A\n";
  EXPECT_EQ(output, expectedOutput);
}
} // namespace facebook::fboss
