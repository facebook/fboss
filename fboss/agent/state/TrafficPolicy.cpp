/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/state/TrafficPolicy.h"
#include "fboss/agent/state/NodeBase-defs.h"
#include "fboss/agent/state/MatchToActionEntry.h"
#include "fboss/agent/state/StateUtils.h"
#include <folly/Conv.h>

namespace {
constexpr auto kMatchToActions = "matchToActions";
constexpr auto kQueues = "queues";
}

namespace facebook { namespace fboss {

folly::dynamic TrafficPolicyFields::toFollyDynamic() const {
  folly::dynamic trafficPolicy = folly::dynamic::object;
  trafficPolicy[kMatchToActions] = folly::dynamic::array;
  for (const auto& mta : matchToActions) {
    trafficPolicy[kMatchToActions].push_back(mta->toFollyDynamic());
  }
  return trafficPolicy;
}

TrafficPolicyFields TrafficPolicyFields::fromFollyDynamic(
    const folly::dynamic& trafficPolicyJson) {
  TrafficPolicyFields trafficPolicy;
  for (const auto& mta : trafficPolicyJson[kMatchToActions]) {
    trafficPolicy.matchToActions.push_back(
        MatchToActionEntry::fromFollyDynamic(mta));
  }

  return trafficPolicy;
}

TrafficPolicy::TrafficPolicy()
  : NodeBaseT() {
}

template class NodeBaseT<TrafficPolicy, TrafficPolicyFields>;

}} // facebook::fboss
