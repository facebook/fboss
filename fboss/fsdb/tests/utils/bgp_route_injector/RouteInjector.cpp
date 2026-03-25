// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/fsdb/tests/utils/bgp_route_injector/RouteInjector.h"

#include <folly/IPAddress.h>
#include <folly/io/async/AsyncSocket.h>
#include <folly/io/async/EventBase.h>
#include <folly/logging/xlog.h>
#include <thrift/lib/cpp2/async/RocketClientChannel.h>

#include "fboss/agent/AddressUtil.h"
#include "fboss/agent/if/gen-cpp2/FbossCtrlAsyncClient.h"
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"

namespace facebook::fboss {

namespace {

// ClientID::BGPD = 0 from ctrl.thrift
constexpr int16_t kBgpClientId = 0;

IpPrefix toIpPrefix(const std::string& prefixStr) {
  auto network = folly::IPAddress::createNetwork(prefixStr);
  IpPrefix ipPrefix;
  ipPrefix.ip() = facebook::network::toBinaryAddress(network.first);
  ipPrefix.prefixLength() = network.second;
  return ipPrefix;
}

NextHopThrift toNextHopThrift(const std::string& nhStr) {
  auto addr = folly::IPAddress(nhStr);
  NextHopThrift nh;
  nh.address() = facebook::network::toBinaryAddress(addr);
  nh.weight() = 0;
  return nh;
}

UnicastRoute toUnicastRoute(const RouteSpec& spec) {
  UnicastRoute route;
  route.dest() = toIpPrefix(spec.prefix);
  route.adminDistance() = AdminDistance::EBGP;
  std::vector<NextHopThrift> nextHops;
  nextHops.reserve(spec.nextHops.size());
  for (const auto& nh : spec.nextHops) {
    nextHops.push_back(toNextHopThrift(nh));
  }
  route.nextHops() = std::move(nextHops);
  return route;
}

std::unique_ptr<FbossCtrlAsyncClient> createClient(
    const std::string& host,
    uint16_t port) {
  folly::EventBase* eb = folly::EventBaseManager::get()->getEventBase();
  folly::SocketAddress addr(host, port);
  auto socket = folly::AsyncSocket::newSocket(eb, addr);
  auto channel =
      apache::thrift::RocketClientChannel::newChannel(std::move(socket));
  return std::make_unique<FbossCtrlAsyncClient>(std::move(channel));
}

} // namespace

RouteInjector::RouteInjector(const std::string& agentHost, uint16_t agentPort)
    : agentHost_(agentHost), agentPort_(agentPort) {
  XLOG(INFO) << "RouteInjector created for " << agentHost << ":" << agentPort;
}

void RouteInjector::addRoute(const RouteSpec& spec) {
  XLOG(INFO) << "Adding route: " << spec.prefix;
  routes_[spec.prefix] = spec;
  try {
    auto client = createClient(agentHost_, agentPort_);
    std::vector<UnicastRoute> routes{toUnicastRoute(spec)};
    client->sync_addUnicastRoutes(kBgpClientId, routes);
    XLOG(INFO) << "Route added successfully";
  } catch (const std::exception& e) {
    XLOG(ERR) << "Failed to add route: " << e.what();
    throw;
  }
}

void RouteInjector::addRoutes(const std::vector<RouteSpec>& specs) {
  XLOG(INFO) << "Adding " << specs.size() << " routes";
  std::vector<UnicastRoute> routes;
  routes.reserve(specs.size());
  for (const auto& spec : specs) {
    routes_[spec.prefix] = spec;
    routes.push_back(toUnicastRoute(spec));
  }
  try {
    auto client = createClient(agentHost_, agentPort_);
    client->sync_addUnicastRoutes(kBgpClientId, routes);
    XLOG(INFO) << "Added " << specs.size() << " routes successfully";
  } catch (const std::exception& e) {
    XLOG(ERR) << "Failed to add routes: " << e.what();
    throw;
  }
}

void RouteInjector::deleteRoute(const std::string& prefixStr) {
  XLOG(INFO) << "Deleting route: " << prefixStr;
  routes_.erase(prefixStr);
  try {
    auto client = createClient(agentHost_, agentPort_);
    std::vector<IpPrefix> prefixes{toIpPrefix(prefixStr)};
    client->sync_deleteUnicastRoutes(kBgpClientId, prefixes);
    XLOG(INFO) << "Route deleted successfully";
  } catch (const std::exception& e) {
    XLOG(ERR) << "Failed to delete route: " << e.what();
    throw;
  }
}

void RouteInjector::deleteAllRoutes() {
  XLOG(INFO) << "Deleting all " << routes_.size() << " routes";
  if (routes_.empty()) {
    return;
  }
  std::vector<IpPrefix> prefixes;
  prefixes.reserve(routes_.size());
  for (const auto& [prefixStr, _] : routes_) {
    prefixes.push_back(toIpPrefix(prefixStr));
  }
  try {
    auto client = createClient(agentHost_, agentPort_);
    client->sync_deleteUnicastRoutes(kBgpClientId, prefixes);
    routes_.clear();
    XLOG(INFO) << "All routes deleted successfully";
  } catch (const std::exception& e) {
    XLOG(ERR) << "Failed to delete routes: " << e.what();
    throw;
  }
}

} // namespace facebook::fboss
