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

#include "fboss/agent/FbossError.h"
#include "fboss/agent/state/RouteTable.h"
#include "fboss/agent/state/RouteTableMap.h"
#include "fboss/agent/state/RouteTableRib.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/ResourceLibUtil.h"

#include <glog/logging.h>
#include <optional>

namespace {

using facebook::fboss::utility::PrefixGenerator;

template <typename AddrT>
folly::CIDRNetwork getNewPrefix(
    PrefixGenerator<AddrT>& prefixGenerator,
    const std::shared_ptr<facebook::fboss::SwitchState>& state,
    facebook::fboss::RouterID routerId) {
  // Obtain a new prefix.
  auto prefix = prefixGenerator.getNext();
  const auto& routeTableRib =
      state->getRouteTables()->getRouteTable(routerId)->getRib<AddrT>();
  while (routeTableRib->exactMatch(prefix)) {
    prefix = prefixGenerator.getNext();
  }
  return folly::CIDRNetwork{{prefix.network}, prefix.mask};
}
} // namespace

namespace facebook::fboss::utility {

RouteDistributionGenerator::RouteDistributionGenerator(
    const std::shared_ptr<SwitchState>& startingState,
    const Masklen2NumPrefixes& v6DistributionSpec,
    const Masklen2NumPrefixes& v4DistributionSpec,
    unsigned int chunkSize,
    unsigned int ecmpWidth,
    RouterID routerId)
    : startingState_(startingState),
      v6DistributionSpec_(v6DistributionSpec),
      v4DistributionSpec_(v4DistributionSpec),
      chunkSize_(chunkSize),
      ecmpWidth_(ecmpWidth),
      routerId_(routerId) {
  CHECK_NE(0, chunkSize_);
  CHECK_NE(0, ecmpWidth_);
}

const RouteDistributionGenerator::RouteChunks& RouteDistributionGenerator::get()
    const {
  if (generatedRouteChunks_) {
    return *generatedRouteChunks_;
  }
  generatedRouteChunks_ = RouteChunks();
  genRouteDistribution<folly::IPAddressV6>(v6DistributionSpec_);
  genRouteDistribution<folly::IPAddressV4>(v4DistributionSpec_);
  return *generatedRouteChunks_;
}

template <typename AddrT>
const std::vector<folly::IPAddress>& RouteDistributionGenerator::getNhops()
    const {
  static std::vector<folly::IPAddress> nhops;
  if (nhops.size()) {
    return nhops;
  }
  EcmpSetupAnyNPorts<AddrT> ecmpHelper(startingState_, routerId_);
  for (auto i = 0; i < ecmpWidth_; ++i) {
    nhops.emplace_back(folly::IPAddress(ecmpHelper.nhop(i).ip));
  }
  return nhops;
}

template <typename AddrT>
void RouteDistributionGenerator::genRouteDistribution(
    const Masklen2NumPrefixes& routeDistribution) const {
  for (const auto& maskLenAndNumPrefixes : routeDistribution) {
    auto prefixGenerator = PrefixGenerator<AddrT>(maskLenAndNumPrefixes.first);
    for (auto i = 0; i < maskLenAndNumPrefixes.second; ++i) {
      if (generatedRouteChunks_->empty() ||
          generatedRouteChunks_->back().size() == chunkSize_) {
        // Last chunk was full or we are just staring.
        // Start a new one
        generatedRouteChunks_->emplace_back(RouteChunk{});
      }
      generatedRouteChunks_->back().emplace_back(
          Route{getNewPrefix(prefixGenerator, startingState_, routerId_),
                getNhops<AddrT>()});
    }
  }
}

const RouteDistributionGenerator::SwitchStates&
RouteDistributionGenerator::getSwitchStates() const {
  if (generatedStates_) {
    return *generatedStates_;
  }
  generatedStates_ = SwitchStates();
  // Resolving next hops is not strictly necessary, since we will
  // program routes even when next hops don't have their ARP/NDP resolved.
  // But for mimicking a more common scenario, resolve these.
  auto ecmpHelper6 = EcmpSetupAnyNPorts6(startingState());
  auto ecmpHelper4 = EcmpSetupAnyNPorts4(startingState());
  auto nhopsResolvedState =
      ecmpHelper6.resolveNextHops(startingState(), ecmpWidth());
  nhopsResolvedState =
      ecmpHelper4.resolveNextHops(nhopsResolvedState, ecmpWidth());
  nhopsResolvedState->publish();
  generatedStates_->push_back(nhopsResolvedState);
  for (const auto& routeChunk : get()) {
    std::vector<RoutePrefixV6> v6Prefixes;
    std::vector<RoutePrefixV4> v4Prefixes;
    for (const auto& route : routeChunk) {
      const auto& cidrNetwork = route.prefix;
      if (cidrNetwork.first.isV6()) {
        v6Prefixes.emplace_back(
            RoutePrefixV6{cidrNetwork.first.asV6(), cidrNetwork.second});
      } else {
        v4Prefixes.emplace_back(
            RoutePrefixV4{cidrNetwork.first.asV4(), cidrNetwork.second});
      }
    }
    auto newState = generatedStates_->back()->clone();
    newState =
        ecmpHelper6.setupECMPForwarding(newState, ecmpWidth(), v6Prefixes);
    newState =
        ecmpHelper4.setupECMPForwarding(newState, ecmpWidth(), v4Prefixes);
    generatedStates_->push_back(newState);
  }

  return *generatedStates_;
}

} // namespace facebook::fboss::utility
