// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <folly/IPAddressV4.h>
#include <folly/IPAddressV6.h>

#include <thrift/lib/cpp2/reflection/testing.h>

#include "fboss/agent/AddressUtil.h"
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"

#include "fboss/cli/fboss2/commands/show/route/CmdShowRouteStatic.h"
#include "fboss/cli/fboss2/commands/show/route/CmdShowRouteStaticIp.h"
#include "fboss/cli/fboss2/commands/show/route/CmdShowRouteStaticIpv6.h"
#include "fboss/cli/fboss2/commands/show/route/CmdShowRouteStaticMpls.h"
#include "fboss/cli/fboss2/test/CmdHandlerTestBase.h"

using namespace ::testing;

namespace facebook::fboss {

namespace {
// action is the agent's forwardActionStr(): "Nexthops" / "Drop" / "ToCPU".
RouteDetails makeRoute(
    const folly::IPAddress& prefix,
    int16_t prefixLength,
    const std::string& action,
    const std::vector<std::string>& nexthops,
    ClientID client = ClientID::STATIC_ROUTE,
    const std::optional<std::string>& ifName = std::nullopt) {
  RouteDetails route;
  IpPrefix dest;
  dest.ip() = facebook::network::toBinaryAddress(prefix);
  dest.prefixLength() = prefixLength;
  route.dest() = dest;
  route.action() = action;
  route.adminDistance() = AdminDistance::STATIC_ROUTE;

  ClientAndNextHops cnh;
  cnh.clientId() = static_cast<int32_t>(client);
  for (const auto& nh : nexthops) {
    NextHopThrift nhop;
    auto addr = facebook::network::toBinaryAddress(folly::IPAddress(nh));
    if (ifName.has_value()) {
      addr.ifName() = *ifName;
    }
    nhop.address() = addr;
    nhop.weight() = 0;
    cnh.nextHops()->emplace_back(nhop);
  }
  route.nextHopMulti()->emplace_back(cnh);
  return route;
}

cli::NextHopInfo makeNhInfo(const std::string& addr) {
  cli::NextHopInfo nh;
  nh.addr() = addr;
  nh.weight() = 0;
  return nh;
}

cli::RouteEntry makeEntry(
    const std::string& prefix,
    const std::vector<cli::NextHopInfo>& nhs) {
  cli::RouteEntry e;
  e.networkAddress() = prefix;
  e.nextHops() = nhs;
  return e;
}
} // namespace

std::vector<RouteDetails> createStaticRoutes() {
  std::vector<RouteDetails> routes;
  routes.emplace_back(makeRoute(
      folly::IPAddress("198.51.100.0"), 24, "Nexthops", {"10.1.1.2"}));
  routes.emplace_back(makeRoute(
      folly::IPAddress("198.51.103.0"),
      24,
      "Nexthops",
      {"10.1.1.2", "10.1.1.3"}));
  routes.emplace_back(
      makeRoute(folly::IPAddress("198.51.101.0"), 24, "Drop", {}));
  routes.emplace_back(
      makeRoute(folly::IPAddress("198.51.102.0"), 24, "ToCPU", {}));
  routes.emplace_back(makeRoute(
      folly::IPAddress("2001:db8:1::"), 64, "Nexthops", {"2001:db8:cafe::2"}));
  return routes;
}

// Golden model for the V4 subset of createStaticRoutes().
cli::ShowRouteModel createV4Model() {
  cli::ShowRouteModel model;
  model.routeEntries() = {
      makeEntry("198.51.100.0/24", {makeNhInfo("10.1.1.2")}),
      makeEntry(
          "198.51.103.0/24", {makeNhInfo("10.1.1.2"), makeNhInfo("10.1.1.3")}),
      makeEntry("198.51.101.0/24", {makeNhInfo("null0")}),
      makeEntry("198.51.102.0/24", {makeNhInfo("cpu")}),
  };
  return model;
}

class CmdShowRouteStaticTestFixture : public CmdHandlerTestBase {
 public:
  std::vector<RouteDetails> routes;

  void SetUp() override {
    CmdHandlerTestBase::SetUp();
    routes = createStaticRoutes();
  }
};

TEST_F(CmdShowRouteStaticTestFixture, createModelV4ThriftEq) {
  auto model = CmdShowRouteStatic::createModel(
      routes, CmdShowRouteStatic::AddressFamily::V4);
  EXPECT_THRIFT_EQ(model, createV4Model());
}

TEST_F(CmdShowRouteStaticTestFixture, createModelAll) {
  auto model = CmdShowRouteStatic::createModel(
      routes, CmdShowRouteStatic::AddressFamily::ALL);
  ASSERT_EQ(model.routeEntries()->size(), 5);
}

TEST_F(CmdShowRouteStaticTestFixture, createModelV6Only) {
  auto model = CmdShowRouteStatic::createModel(
      routes, CmdShowRouteStatic::AddressFamily::V6);
  ASSERT_EQ(model.routeEntries()->size(), 1);
  EXPECT_EQ(model.routeEntries()->at(0).networkAddress(), "2001:db8:1::/64");
  ASSERT_EQ(model.routeEntries()->at(0).nextHops()->size(), 1);
  EXPECT_EQ(
      model.routeEntries()->at(0).nextHops()->at(0).addr(), "2001:db8:cafe::2");
}

TEST_F(CmdShowRouteStaticTestFixture, emptyResult) {
  std::vector<RouteDetails> empty;
  auto model = CmdShowRouteStatic::createModel(
      empty, CmdShowRouteStatic::AddressFamily::ALL);
  EXPECT_TRUE(model.routeEntries()->empty());

  std::stringstream ss;
  CmdShowRouteStatic().printOutput(model, ss);
  EXPECT_EQ(ss.str(), "");
}

TEST_F(CmdShowRouteStaticTestFixture, nextHopAddrsFallback) {
  // A static route whose nexthops live only in the deprecated nextHopAddrs
  // field must still render its nexthop (not be mislabelled as null0).
  RouteDetails route;
  IpPrefix dest;
  dest.ip() = facebook::network::toBinaryAddress(folly::IPAddress("10.0.0.0"));
  dest.prefixLength() = 8;
  route.dest() = dest;
  route.action() = "Nexthops";
  ClientAndNextHops cnh;
  cnh.clientId() = static_cast<int32_t>(ClientID::STATIC_ROUTE);
  cnh.nextHopAddrs() = {
      facebook::network::toBinaryAddress(folly::IPAddress("10.1.1.2"))};
  route.nextHopMulti()->emplace_back(cnh);

  auto model = CmdShowRouteStatic::createModel(
      {route}, CmdShowRouteStatic::AddressFamily::V4);
  ASSERT_EQ(model.routeEntries()->size(), 1);
  ASSERT_EQ(model.routeEntries()->at(0).nextHops()->size(), 1);
  EXPECT_EQ(model.routeEntries()->at(0).nextHops()->at(0).addr(), "10.1.1.2");
}

TEST_F(CmdShowRouteStaticTestFixture, interfaceNextHop) {
  // Interface-bound nexthop (addr.ifName set) renders as "<addr> dev <ifName>".
  auto route = makeRoute(
      folly::IPAddress("198.51.50.0"),
      24,
      "Nexthops",
      {"10.1.1.2"},
      ClientID::STATIC_ROUTE,
      "fboss4094");
  auto model = CmdShowRouteStatic::createModel(
      {route}, CmdShowRouteStatic::AddressFamily::V4);
  ASSERT_EQ(model.routeEntries()->size(), 1);
  EXPECT_EQ(
      model.routeEntries()->at(0).nextHops()->at(0).ifName(), "fboss4094");

  std::stringstream ss;
  CmdShowRouteStatic().printOutput(model, ss);
  EXPECT_THAT(ss.str(), HasSubstr("via 10.1.1.2 dev fboss4094"));
}

TEST_F(CmdShowRouteStaticTestFixture, nullAndCpuNextHops) {
  auto model = CmdShowRouteStatic::createModel(
      routes, CmdShowRouteStatic::AddressFamily::V4);
  std::map<std::string, std::vector<std::string>> prefixToNhs;
  for (const auto& entry : model.routeEntries().value()) {
    for (const auto& nh : entry.nextHops().value()) {
      prefixToNhs[*entry.networkAddress()].push_back(*nh.addr());
    }
  }
  // null0 (Drop) and cpu (ToCPU) must be distinguished by the action.
  ASSERT_EQ(prefixToNhs.count("198.51.101.0/24"), 1u);
  EXPECT_THAT(prefixToNhs["198.51.101.0/24"], ElementsAre("null0"));
  ASSERT_EQ(prefixToNhs.count("198.51.102.0/24"), 1u);
  EXPECT_THAT(prefixToNhs["198.51.102.0/24"], ElementsAre("cpu"));
  ASSERT_EQ(prefixToNhs.count("198.51.100.0/24"), 1u);
  EXPECT_THAT(prefixToNhs["198.51.100.0/24"], ElementsAre("10.1.1.2"));
  // ECMP route keeps both nexthops.
  ASSERT_EQ(prefixToNhs.count("198.51.103.0/24"), 1u);
  EXPECT_THAT(
      prefixToNhs["198.51.103.0/24"], ElementsAre("10.1.1.2", "10.1.1.3"));
}

TEST_F(CmdShowRouteStaticTestFixture, queryClientFiltersNonStatic) {
  // Add a BGP route which must be filtered out by the STATIC_ROUTE filter.
  routes.emplace_back(makeRoute(
      folly::IPAddress("10.10.10.0"),
      24,
      "Nexthops",
      {"10.1.1.9"},
      ClientID::BGPD));

  setupMockedAgentServer();
  EXPECT_CALL(getMockAgent(), getRouteTableDetails(_))
      .WillOnce(Invoke([&](auto& entries) { entries = routes; }));

  auto cmd = CmdShowRouteStaticIp();
  auto model = cmd.queryClient(localhost());
  EXPECT_THRIFT_EQ(model, createV4Model()); // 4 static v4 routes; BGP excluded
}

TEST_F(CmdShowRouteStaticTestFixture, queryClientIpv6) {
  setupMockedAgentServer();
  EXPECT_CALL(getMockAgent(), getRouteTableDetails(_))
      .WillOnce(Invoke([&](auto& entries) { entries = routes; }));

  auto cmd = CmdShowRouteStaticIpv6();
  auto model = cmd.queryClient(localhost());
  EXPECT_EQ(model.routeEntries()->size(), 1);
}

TEST_F(CmdShowRouteStaticTestFixture, printOutput) {
  auto model = CmdShowRouteStatic::createModel(
      routes, CmdShowRouteStatic::AddressFamily::V4);
  std::stringstream ss;
  CmdShowRouteStatic().printOutput(model, ss);
  std::string out = ss.str();
  EXPECT_THAT(out, HasSubstr("Network Address: 198.51.100.0/24"));
  EXPECT_THAT(out, HasSubstr("via 10.1.1.2"));
  EXPECT_THAT(out, HasSubstr("via null0"));
  EXPECT_THAT(out, HasSubstr("via cpu"));
}

class CmdShowRouteStaticMplsTestFixture : public CmdHandlerTestBase {
 public:
  std::vector<MplsRoute> mplsRoutes;

  void SetUp() override {
    CmdHandlerTestBase::SetUp();
    MplsRoute mplsRoute;
    mplsRoute.topLabel() = 1001;
    mplsRoute.adminDistance() = AdminDistance::STATIC_ROUTE;
    NextHopThrift nhop;
    nhop.address() =
        facebook::network::toBinaryAddress(folly::IPAddress("10.1.1.2"));
    MplsAction action;
    action.action() = MplsActionCode::PUSH;
    action.pushLabels() = std::vector<int32_t>{2001, 2002};
    nhop.mplsAction() = action;
    mplsRoute.nextHops()->emplace_back(nhop);
    mplsRoutes.emplace_back(mplsRoute);
  }
};

TEST_F(CmdShowRouteStaticMplsTestFixture, createModelPushLabels) {
  auto model = CmdShowRouteStaticMpls::createModel(mplsRoutes);
  ASSERT_EQ(model.mplsEntries()->size(), 1);
  EXPECT_EQ(model.mplsEntries()->at(0).topLabel(), 1001);

  std::stringstream ss;
  CmdShowRouteStaticMpls().printOutput(model, ss);
  std::string out = ss.str();
  EXPECT_THAT(out, HasSubstr("MPLS Label: 1001"));
  EXPECT_THAT(out, HasSubstr("2001,2002"));
}

TEST_F(CmdShowRouteStaticMplsTestFixture, queryClient) {
  setupMockedAgentServer();
  EXPECT_CALL(getMockAgent(), getMplsRouteTableByClient(_, _))
      .WillOnce(Invoke([&](auto& entries, int16_t clientId) {
        EXPECT_EQ(clientId, static_cast<int16_t>(ClientID::STATIC_ROUTE));
        entries = mplsRoutes;
      }));

  auto cmd = CmdShowRouteStaticMpls();
  auto model = cmd.queryClient(localhost());
  ASSERT_EQ(model.mplsEntries()->size(), 1);
  EXPECT_EQ(model.mplsEntries()->at(0).topLabel(), 1001);
}

} // namespace facebook::fboss
