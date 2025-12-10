/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/test/RouteDistributionGenerator.h"

#include "fboss/agent/Utils.h"
#include "fboss/agent/test/EcmpSetupHelper.h"

#include <glog/logging.h>
#include <optional>

namespace facebook::fboss {
namespace {
UnicastRoute makeUnicastRoute(
    const folly::CIDRNetwork& nw,
    const std::vector<UnresolvedNextHop>& nhops,
    AdminDistance admin = AdminDistance::EBGP,
    std::optional<RouteCounterID> counterID = std::nullopt) {
  UnicastRoute route;
  route.dest() = facebook::fboss::toIpPrefix(nw);
  route.action() = RouteForwardAction::NEXTHOPS;
  route.adminDistance() = admin;
  std::vector<NextHopThrift> nhopsThrift;
  nhopsThrift.reserve(nhops.size());
  std::for_each(nhops.begin(), nhops.end(), [&nhopsThrift](const auto& nhop) {
    nhopsThrift.emplace_back(NextHop(nhop).toThrift());
  });
  route.nextHops() = nhopsThrift;
  if (counterID.has_value()) {
    route.counterID() = *counterID;
  }
  return route;
}
} // namespace
} // namespace facebook::fboss

namespace facebook::fboss::utility {

RouteDistributionGenerator::RouteDistributionGenerator(
    const std::shared_ptr<SwitchState>& startingState,
    const RouteDistributionSpec<folly::IPAddressV6>& v6DistributionSpec,
    const RouteDistributionSpec<folly::IPAddressV4>& v4DistributionSpec,
    unsigned int chunkSize,
    unsigned int ecmpWidth,
    bool needL2EntryForNeighbor,
    RouterID routerId)
    : startingState_(startingState),
      v6DistributionSpec_(v6DistributionSpec),
      v4DistributionSpec_(v4DistributionSpec),
      chunkSize_(chunkSize),
      ecmpWidth_(ecmpWidth),
      needL2EntryForNeighbor_(needL2EntryForNeighbor),
      routerId_(routerId) {
  CHECK_NE(0, chunkSize_);
  CHECK_NE(0, ecmpWidth_);
}

std::shared_ptr<SwitchState> RouteDistributionGenerator::resolveNextHops(
    std::shared_ptr<SwitchState> in) const {
  auto nhop6Resolved = EcmpSetupAnyNPorts6(in, needL2EntryForNeighbor_)
                           .resolveNextHops(in, ecmpWidth());
  return EcmpSetupAnyNPorts4(nhop6Resolved, needL2EntryForNeighbor_)
      .resolveNextHops(nhop6Resolved, ecmpWidth());
}

const RouteDistributionGenerator::RouteChunks& RouteDistributionGenerator::get()
    const {
  genRoutes();
  return *generatedRouteChunks_;
}

void RouteDistributionGenerator::genRoutes() const {
  if (generatedRouteChunks_) {
    return;
  }
  generatedRouteChunks_ = RouteChunks();
  genRouteDistribution<folly::IPAddressV6>(v6DistributionSpec_);
  genRouteDistribution<folly::IPAddressV4>(v4DistributionSpec_);
}

const RouteDistributionGenerator::ThriftRouteChunks&
RouteDistributionGenerator::getThriftRoutes(
    std::optional<RouteCounterID> counterID) const {
  if (generatedThriftRoutes_) {
    return *generatedThriftRoutes_;
  }
  generatedThriftRoutes_ = ThriftRouteChunks();
  for (const auto& routeChunk : get()) {
    ThriftRouteChunk thriftRoutes;
    std::for_each(
        routeChunk.begin(),
        routeChunk.end(),
        [&thriftRoutes, &counterID](const auto& route) {
          thriftRoutes.emplace_back(makeUnicastRoute(
              route.prefix, route.nhops, AdminDistance::EBGP, counterID));
        });
    generatedThriftRoutes_->push_back(std::move(thriftRoutes));
  }

  return *generatedThriftRoutes_;
}

RouteDistributionGenerator::RouteChunk RouteDistributionGenerator::allRoutes()
    const {
  RouteChunk routes;
  std::for_each(get().begin(), get().end(), [&routes](const auto& chunk) {
    routes.insert(routes.end(), chunk.begin(), chunk.end());
  });
  return routes;
}

RouteDistributionGenerator::ThriftRouteChunk
RouteDistributionGenerator::allThriftRoutes() const {
  ThriftRouteChunk routes;
  std::for_each(
      getThriftRoutes().begin(),
      getThriftRoutes().end(),
      [&routes](const auto& chunk) {
        routes.insert(routes.end(), chunk.begin(), chunk.end());
      });
  return routes;
}

template <typename AddrT>
const std::vector<UnresolvedNextHop>& RouteDistributionGenerator::getNhops()
    const {
  static std::vector<UnresolvedNextHop> nhops;
  if (nhops.size()) {
    return nhops;
  }
  EcmpSetupAnyNPorts<AddrT> ecmpHelper(startingState_, routerId_);
  CHECK(ecmpWidth_ <= ecmpHelper.getNextHops().size());
  for (auto i = 0; i < ecmpWidth_; ++i) {
    nhops.emplace_back(folly::IPAddress(ecmpHelper.nhop(i).ip), ECMP_WEIGHT);
  }
  return nhops;
}

template <typename AddrT>
void RouteDistributionGenerator::genRouteDistribution(
    const RouteDistributionSpec<AddrT>& routeDistributionSpec) const {
  for (const auto& routeDistribution : routeDistributionSpec) {
    auto prefixGenerator = PrefixGenerator<AddrT>(routeDistribution.masklen);
    for (auto i = 0; i < routeDistribution.numPrefixes; ++i) {
      if (generatedRouteChunks_->empty() ||
          generatedRouteChunks_->back().size() == chunkSize_) {
        // Last chunk was full or we are just staring.
        // Start a new one
        generatedRouteChunks_->emplace_back();
      }
      generatedRouteChunks_->back().emplace_back(
          Route{
              getNewPrefix(
                  prefixGenerator,
                  startingState_,
                  routerId_,
                  routeDistribution.offset),
              getNhops<AddrT>()});
    }
  }
}

} // namespace facebook::fboss::utility
