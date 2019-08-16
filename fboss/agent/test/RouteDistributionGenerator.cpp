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

#include <folly/Optional.h>
#include <glog/logging.h>

namespace {

using facebook::fboss::utility::PrefixGenerator;

template <typename AddrT>
typename PrefixGenerator<AddrT>::ResourceT genNewPrefix(
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
  return prefix;
}
} // namespace

namespace facebook {
namespace fboss {
namespace utility {

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

RouteDistributionGenerator::SwitchStates RouteDistributionGenerator::get() {
  if (generatedStates_) {
    return *generatedStates_;
  }
  generatedStates_ = SwitchStates();
  // Resolving next hops is not strictly necessary, since we will
  // program routes even when next hops don't have their ARP/NDP resolved.
  // But for mimicking a more common scenario, resolve these.
  auto nhopsResolvedState = EcmpSetupAnyNPorts6(startingState_)
                                .resolveNextHops(startingState_, ecmpWidth_);
  nhopsResolvedState = EcmpSetupAnyNPorts4(nhopsResolvedState)
                           .resolveNextHops(nhopsResolvedState, ecmpWidth_);
  nhopsResolvedState->publish();
  generatedStates_->push_back(nhopsResolvedState);
  int trailingUpdateSize = 0;
  genRouteDistribution<folly::IPAddressV6>(
      v6DistributionSpec_, &trailingUpdateSize);
  genRouteDistribution<folly::IPAddressV4>(
      v4DistributionSpec_, &trailingUpdateSize);
  return *generatedStates_;
}

template <typename AddrT>
void RouteDistributionGenerator::genRouteDistribution(
    const Masklen2NumPrefixes& routeDistribution,
    int* trailingUpdateSize) {
  std::shared_ptr<SwitchState> curState;
  CHECK(!generatedStates_->empty());
  if (*trailingUpdateSize != 0) {
    // If trailingUpdateSize was non zero, it means that the
    // last update did not fillup to chunk size. Continue filling
    // that one.
    curState = generatedStates_->back();
    generatedStates_->pop_back();
  } else {
    // Else last update was full or we are just staring.
    curState = generatedStates_->back()->clone();
  }
  std::vector<typename PrefixGenerator<AddrT>::ResourceT> prefixChunk;
  EcmpSetupAnyNPorts<AddrT> ecmpHelper(curState, routerId_);
  for (const auto& maskLenAndNumPrefixes : routeDistribution) {
    auto prefixGenerator = PrefixGenerator<AddrT>(maskLenAndNumPrefixes.first);
    for (auto i = 0; i < maskLenAndNumPrefixes.second; ++i) {
      prefixChunk.emplace_back(
          genNewPrefix(prefixGenerator, curState, routerId_));
      if (++*trailingUpdateSize % chunkSize_ == 0) {
        curState =
            ecmpHelper.setupECMPForwarding(curState, ecmpWidth_, prefixChunk);
        // If curState has added chunkSize routes, publish
        // it and start fillin in a new state
        curState->publish();
        generatedStates_->push_back(curState);
        curState = generatedStates_->back()->clone();
        *trailingUpdateSize = 0;
        prefixChunk.clear();
      }
    }
  }
  if (*trailingUpdateSize % chunkSize_) {
    // We are done adding routes for this distribution. Add the
    // state update if we didn't add it in already (by reaching
    // chunk size)
    curState =
        ecmpHelper.setupECMPForwarding(curState, ecmpWidth_, prefixChunk);
    generatedStates_->push_back(curState);
  }
}
} // namespace utility
} // namespace fboss
} // namespace facebook
