// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <folly/IPAddress.h>
#include <folly/IPAddressV6.h>

#include "fboss/agent/AddressUtil.h"
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"

#include "fboss/cli/fboss2/CmdGlobalOptions.h"
#include "fboss/cli/fboss2/commands/show/route/CmdShowRoute.h"
#include "fboss/cli/fboss2/test/CmdHandlerTestBase.h"
#include "fboss/cli/fboss2/utils/FilterUtils.h"

using namespace ::testing;

namespace facebook::fboss {

namespace {
RouteDetails makeRoute(
    const folly::IPAddress& prefix,
    int16_t prefixLength,
    const std::vector<std::string>& fwdNexthops,
    const std::vector<ClientID>& clients,
    bool isConnected = false) {
  RouteDetails route;
  IpPrefix dest;
  dest.ip() = facebook::network::toBinaryAddress(prefix);
  dest.prefixLength() = prefixLength;
  route.dest() = dest;
  route.action() = fwdNexthops.empty() ? "Drop" : "Nexthops";
  route.isConnected() = isConnected;

  for (const auto& nh : fwdNexthops) {
    NextHopThrift nhop;
    nhop.address() = facebook::network::toBinaryAddress(folly::IPAddress(nh));
    nhop.weight() = 0;
    route.nextHops()->emplace_back(nhop);
  }
  for (const auto& client : clients) {
    ClientAndNextHops cnh;
    cnh.clientId() = static_cast<int32_t>(client);
    for (const auto& nh : fwdNexthops) {
      NextHopThrift nhop;
      nhop.address() = facebook::network::toBinaryAddress(folly::IPAddress(nh));
      nhop.weight() = 0;
      cnh.nextHops()->emplace_back(nhop);
    }
    route.nextHopMulti()->emplace_back(cnh);
  }
  return route;
}

CmdGlobalOptions::UnionList makeFilter(
    const std::string& key,
    const std::string& value) {
  CmdGlobalOptions::FilterTerm term = {
      key, std::make_shared<FilterOpEq>(), value};
  return {{term}};
}
} // namespace

class CmdShowRouteTestFixture : public CmdHandlerTestBase {
 public:
  std::vector<RouteDetails> routes;

  void SetUp() override {
    CmdHandlerTestBase::SetUp();
    // static v4, bgp v6, connected v4, bgp-vs-static-internal default route,
    // bgp-vs-static prefix (static wins on admin distance).
    routes.emplace_back(makeRoute(
        folly::IPAddress("198.51.100.0"),
        24,
        {"10.99.1.2"},
        {ClientID::STATIC_ROUTE}));
    routes.emplace_back(makeRoute(
        folly::IPAddress("2001:db8:1::"),
        64,
        {"2001:db8:99::2"},
        {ClientID::BGPD}));
    routes.emplace_back(makeRoute(
        folly::IPAddress("10.99.1.0"),
        24,
        {},
        {ClientID::INTERFACE_ROUTE},
        true /* isConnected */));
    routes.emplace_back(makeRoute(
        folly::IPAddress("::"),
        0,
        {"2001:db8:99::2"},
        {ClientID::STATIC_INTERNAL, ClientID::BGPD}));
    routes.emplace_back(makeRoute(
        folly::IPAddress("192.168.0.0"),
        16,
        {"10.99.1.2"},
        {ClientID::BGPD, ClientID::STATIC_ROUTE}));
  }

  std::map<std::string, cli::RouteEntry> entriesByPrefix(
      const cli::ShowRouteModel& model) {
    std::map<std::string, cli::RouteEntry> byPrefix;
    for (const auto& entry : model.routeEntries().value()) {
      byPrefix[*entry.networkAddress()] = entry;
    }
    return byPrefix;
  }
};

TEST_F(CmdShowRouteTestFixture, createModelProtocolAndFamily) {
  auto model = CmdShowRoute().createModel(routes);
  auto byPrefix = entriesByPrefix(model);
  ASSERT_EQ(byPrefix.size(), 5u);

  EXPECT_EQ(*byPrefix.at("198.51.100.0/24").protocol(), "static");
  EXPECT_EQ(*byPrefix.at("198.51.100.0/24").addressFamily(), "ipv4");

  EXPECT_EQ(*byPrefix.at("2001:db8:1::/64").protocol(), "bgp");
  EXPECT_EQ(*byPrefix.at("2001:db8:1::/64").addressFamily(), "ipv6");

  EXPECT_EQ(*byPrefix.at("10.99.1.0/24").protocol(), "connected");

  // Multi-client prefixes resolve to the lowest-admin-distance client.
  EXPECT_EQ(*byPrefix.at("::/0").protocol(), "bgp");
  EXPECT_EQ(*byPrefix.at("192.168.0.0/16").protocol(), "static");
}

TEST_F(CmdShowRouteTestFixture, validFilterKeys) {
  auto validFilters = CmdShowRoute().getValidFilters();
  EXPECT_EQ(validFilters.count("networkAddress"), 1u);
  EXPECT_EQ(validFilters.count("protocol"), 1u);
  EXPECT_EQ(validFilters.count("addressFamily"), 1u);
  // List fields are not filterable.
  EXPECT_EQ(validFilters.count("nextHops"), 0u);
}

TEST_F(CmdShowRouteTestFixture, filterByProtocol) {
  auto model = CmdShowRoute().createModel(routes);
  auto validFilters = CmdShowRoute().getValidFilters();

  auto filtered = filterOutput<CmdShowRoute>(
      model, makeFilter("protocol", "static"), validFilters);
  auto byPrefix = entriesByPrefix(filtered);
  EXPECT_EQ(byPrefix.size(), 2u);
  EXPECT_EQ(byPrefix.count("198.51.100.0/24"), 1u);
  EXPECT_EQ(byPrefix.count("192.168.0.0/16"), 1u);

  filtered = filterOutput<CmdShowRoute>(
      model, makeFilter("protocol", "connected"), validFilters);
  byPrefix = entriesByPrefix(filtered);
  EXPECT_EQ(byPrefix.size(), 1u);
  EXPECT_EQ(byPrefix.count("10.99.1.0/24"), 1u);
}

TEST_F(CmdShowRouteTestFixture, filterByAddressFamily) {
  auto model = CmdShowRoute().createModel(routes);
  auto validFilters = CmdShowRoute().getValidFilters();

  auto filtered = filterOutput<CmdShowRoute>(
      model, makeFilter("addressFamily", "ipv6"), validFilters);
  auto byPrefix = entriesByPrefix(filtered);
  EXPECT_EQ(byPrefix.size(), 2u);
  EXPECT_EQ(byPrefix.count("2001:db8:1::/64"), 1u);
  EXPECT_EQ(byPrefix.count("::/0"), 1u);
}

TEST_F(CmdShowRouteTestFixture, filterIntersectionAndUnion) {
  auto model = CmdShowRoute().createModel(routes);
  auto validFilters = CmdShowRoute().getValidFilters();

  // protocol == bgp && addressFamily == ipv6
  CmdGlobalOptions::FilterTerm t1 = {
      "protocol", std::make_shared<FilterOpEq>(), "bgp"};
  CmdGlobalOptions::FilterTerm t2 = {
      "addressFamily", std::make_shared<FilterOpEq>(), "ipv6"};
  CmdGlobalOptions::UnionList intersection = {{t1, t2}};
  auto filtered = filterOutput<CmdShowRoute>(model, intersection, validFilters);
  auto byPrefix = entriesByPrefix(filtered);
  EXPECT_EQ(byPrefix.size(), 2u);
  EXPECT_EQ(byPrefix.count("2001:db8:1::/64"), 1u);
  EXPECT_EQ(byPrefix.count("::/0"), 1u);

  // protocol == connected || protocol == static
  CmdGlobalOptions::FilterTerm t3 = {
      "protocol", std::make_shared<FilterOpEq>(), "connected"};
  CmdGlobalOptions::FilterTerm t4 = {
      "protocol", std::make_shared<FilterOpEq>(), "static"};
  CmdGlobalOptions::UnionList unionFilter = {{t3}, {t4}};
  filtered = filterOutput<CmdShowRoute>(model, unionFilter, validFilters);
  EXPECT_EQ(filtered.routeEntries()->size(), 3u);
}

TEST_F(CmdShowRouteTestFixture, queryClient) {
  setupMockedAgentServer();
  EXPECT_CALL(getMockAgent(), getRouteTableDetails(_))
      .WillOnce(Invoke([&](auto& entries) { entries = routes; }));

  auto cmd = CmdShowRoute();
  auto model = cmd.queryClient(localhost());
  EXPECT_EQ(model.routeEntries()->size(), 5u);
}

TEST_F(CmdShowRouteTestFixture, printOutput) {
  auto model = CmdShowRoute().createModel(routes);
  std::stringstream ss;
  CmdShowRoute().printOutput(model, ss);
  std::string out = ss.str();
  EXPECT_THAT(out, HasSubstr("Network Address: 198.51.100.0/24"));
  EXPECT_THAT(out, HasSubstr("via 10.99.1.2"));
}

TEST_F(CmdShowRouteTestFixture, annotatesBackupNextHops) {
  RouteDetails route;
  IpPrefix dest;
  dest.ip() =
      facebook::network::toBinaryAddress(folly::IPAddressV6("2401:db00::"));
  dest.prefixLength() = 64;
  route.dest() = dest;
  route.action() = "Nexthops";

  NextHopThrift primary;
  primary.address() =
      facebook::network::toBinaryAddress(folly::IPAddressV6("1::1"));

  NextHopThrift backup;
  backup.address() =
      facebook::network::toBinaryAddress(folly::IPAddressV6("2::1"));
  backup.role() = NextHopRole::BACKUP;

  route.nextHops() = {primary, backup};
  ClientAndNextHops cnh;
  cnh.clientId() = static_cast<int32_t>(ClientID::BGPD);
  cnh.nextHops() = {primary, backup};
  route.nextHopMulti()->emplace_back(cnh);

  std::vector<RouteDetails> backupRoutes{route};
  auto cmd = CmdShowRoute();
  auto model = cmd.createModel(backupRoutes);
  std::stringstream output;
  cmd.printOutput(model, output);

  EXPECT_EQ(
      output.str(),
      "Network Address: 2401:db00::/64\n"
      "\tvia 1::1\n"
      "\tvia 2::1 (BACKUP)\n");
}

// CLI reference wiki hooks: a human description and a non-empty sample model.
// Property checks only (no golden text).
TEST_F(CmdShowRouteTestFixture, wikiDocHooks) {
  EXPECT_FALSE(CmdShowRouteTraits::description().empty());
  EXPECT_FALSE(CmdShowRoute::sampleModel().routeEntries()->empty());
}

} // namespace facebook::fboss
