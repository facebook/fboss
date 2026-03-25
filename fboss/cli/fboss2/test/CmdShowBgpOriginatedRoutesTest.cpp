// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <fmt/core.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <sstream>
#include <string>
#include "fboss/cli/fboss2/test/CmdHandlerTestBase.h"

#include "common/network/if/gen-cpp2/Address_types.h"
#include "configerator/structs/neteng/fboss/bgp/if/gen-cpp2/bgp_attr_types.h"
#include "fboss/agent/AddressUtil.h"
#include "fboss/bgp/if/gen-cpp2/bgp_thrift_types.h"
#include "fboss/cli/fboss2/commands/show/bgp/CmdShowBgpOriginatedRoutes.h"
#include "folly/IPAddress.h"
#include "folly/Range.h"
#include "folly/json/json.h"

using namespace ::testing;
using facebook::neteng::fboss::bgp::thrift::TOriginatedRoute;
using facebook::neteng::fboss::bgp_attr::TBgpAfi;
using facebook::neteng::fboss::bgp_attr::TBgpCommunity;
using facebook::neteng::fboss::bgp_attr::TIpPrefix;
namespace facebook::fboss {

const auto kIpVersion = TBgpAfi::AFI_IPV4;
const auto kBinaryAddress =
    facebook::network::toBinaryAddress(folly::IPAddress("8.0.0.0"));
const auto kAddressMask = 32;
const auto kCommunityNumber = 4294390177;
const auto kSupportingRoutes = 0;

class CmdShowBgpOriginatedRoutesTestFixture : public CmdHandlerTestBase {
 public:
  std::vector<TOriginatedRoute> routes_;
  void SetUp() override {
    CmdHandlerTestBase::SetUp();
    routes_ = getOriginatedRoutes();
  }

  std::vector<TOriginatedRoute> getOriginatedRoutes() {
    TIpPrefix ip_prefix;
    ip_prefix.afi() = kIpVersion;
    ip_prefix.prefix_bin() = kBinaryAddress.addr().value().toStdString();
    ip_prefix.num_bits() = kAddressMask;

    TBgpCommunity bgp_community;
    bgp_community.community() = kCommunityNumber;

    TOriginatedRoute queried_route;
    queried_route.prefix() = ip_prefix;
    queried_route.communities() = {bgp_community};
    queried_route.supporting_route_count() = kSupportingRoutes;
    queried_route.minimum_supporting_routes() = kSupportingRoutes;

    return {queried_route};
  }
};

TEST_F(CmdShowBgpOriginatedRoutesTestFixture, queryClient) {
  setupMockedBgpServer();
  EXPECT_CALL(getMockBgp(), getOriginatedRoutes(_))
      .WillOnce(Invoke([&](auto& entries) { entries = routes_; }));

  auto results = CmdShowBgpOriginatedRoutes().queryClient(localhost());
  const auto routes = *results.tOriginatedRoutes();
  ASSERT_EQ(routes.size(), 1);

  const auto& prefix = routes[0].prefix().value();
  EXPECT_EQ(prefix.get_afi(), kIpVersion);
  EXPECT_EQ(prefix.get_prefix_bin(), kBinaryAddress.addr()->toStdString());
  EXPECT_EQ(prefix.get_num_bits(), kAddressMask);

  const auto& communites =
      apache::thrift::get_pointer(routes[0].communities())[0];
  ASSERT_EQ(communites.size(), 1);
  EXPECT_EQ(communites[0].get_community(), kCommunityNumber);
  EXPECT_EQ(routes[0].get_supporting_route_count(), kSupportingRoutes);
  EXPECT_EQ(routes[0].get_minimum_supporting_routes(), kSupportingRoutes);
}

TEST_F(CmdShowBgpOriginatedRoutesTestFixture, printOutput) {
  setupMockedBgpServer();
  EXPECT_CALL(getMockBgp(), getRunningConfig(_))
      .WillOnce(Invoke([&](std::string& config) {
        // clang-format off
        folly::dynamic value = folly::dynamic::object
          ("communities",
          folly::dynamic::array(
          folly::dynamic::object("name", "FABRIC_POD_RSW_LOOP")
          ("description", "rsw loopback")
          ("communities", folly::dynamic::array("65527:12705"))
          )
        );
        // clang-format on
        config = folly::toPrettyJson(value);
      }));
  std::stringstream ss;
  TOriginatedRouteWithHost originatedRouteWithHost;
  originatedRouteWithHost.tOriginatedRoutes() = routes_;
  originatedRouteWithHost.host() = localhost().getName();
  originatedRouteWithHost.oobName() = localhost().getOobName();
  originatedRouteWithHost.ip() = localhost().getIpStr();
  CmdShowBgpOriginatedRoutes().printOutput(originatedRouteWithHost, ss);

  std::string output = ss.str();
  std::string expectedOutput =
      " Prefix      Communities                      Supporting Route Cnt  Minimum supporting route  Require Nexthop Resolution \n"
      "-------------------------------------------------------------------------------------------------------------------------------\n"
      " 8.0.0.0/32  FABRIC_POD_RSW_LOOP/65527:12705  0                     0                         N/A                        \n\n";

  EXPECT_EQ(output, expectedOutput);
}
} // namespace facebook::fboss
