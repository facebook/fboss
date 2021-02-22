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
#include "fboss/agent/test/EcmpSetupHelper.h"

#include <glog/logging.h>
#include <optional>

namespace {

using facebook::fboss::utility::PrefixGenerator;

} // namespace

namespace facebook::fboss::utility {

RouteDistributionGenerator::RouteDistributionGenerator(
    const std::shared_ptr<SwitchState>& startingState,
    const Masklen2NumPrefixes& v6DistributionSpec,
    const Masklen2NumPrefixes& v4DistributionSpec,
    bool isStandaloneRibEnabled,
    unsigned int chunkSize,
    unsigned int ecmpWidth,
    RouterID routerId)
    : startingState_(startingState),
      v6DistributionSpec_(v6DistributionSpec),
      v4DistributionSpec_(v4DistributionSpec),
      isStandaloneRibEnabled_(isStandaloneRibEnabled),
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
      generatedRouteChunks_->back().emplace_back(Route{
          getNewPrefix(
              prefixGenerator,
              startingState_,
              routerId_,
              isStandaloneRibEnabled_),
          getNhops<AddrT>()});
    }
  }
}

} // namespace facebook::fboss::utility
