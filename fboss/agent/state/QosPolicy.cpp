/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/state/QosPolicy.h"
#include <folly/Conv.h>
#include "fboss/agent/state/NodeBase-defs.h"
#include "fboss/agent/state/StateUtils.h"

namespace {
constexpr auto kQueueId = "queueId";
constexpr auto kDscp = "dscp";
constexpr auto kExp = "exp";
constexpr auto kRules = "rules";
constexpr auto kName = "name";
constexpr auto kTrafficClass = "trafficClass";
constexpr auto kDscpMap = "dscpMap";
constexpr auto kExpMap = "expMap";
constexpr auto kTrafficClassToQueueId = "trafficClassToQueueId";
constexpr auto kPfcPriorityToQueueId = "pfcPriorityToQueueId";
constexpr auto kPfcPriority = "pfcPriority";
constexpr auto kFrom = "from";
constexpr auto kTo = "to";
constexpr auto kTrafficClassToPgId = "trafficClassToPgId";
constexpr auto kPgId = "pgId";
constexpr auto kPfcPriorityToPgId = "pfcPriorityToPgId";
constexpr auto kAttr = "attr";

} // namespace

namespace facebook::fboss {

DscpMap::DscpMap(std::vector<cfg::DscpQosMap> cfg) {
  for (auto map : cfg) {
    auto trafficClass = *map.internalTrafficClass();

    for (auto dscp : *map.fromDscpToTrafficClass()) {
      addFromEntry(
          static_cast<TrafficClass>(trafficClass), static_cast<DSCP>(dscp));
    }
    if (auto fromTrafficClassToDscp = map.fromTrafficClassToDscp()) {
      addToEntry(
          static_cast<TrafficClass>(trafficClass),
          static_cast<DSCP>(fromTrafficClassToDscp.value()));
    }
  }
}

ExpMap::ExpMap(std::vector<cfg::ExpQosMap> cfg) {
  for (auto map : cfg) {
    auto trafficClass = *map.internalTrafficClass();

    for (auto exp : *map.fromExpToTrafficClass()) {
      addFromEntry(
          static_cast<TrafficClass>(trafficClass), static_cast<EXP>(exp));
    }
    if (auto fromTrafficClassToExp = map.fromTrafficClassToExp()) {
      addToEntry(
          static_cast<TrafficClass>(trafficClass),
          static_cast<EXP>(fromTrafficClassToExp.value()));
    }
  }
}

template class ThriftStructNode<QosPolicy, state::QosPolicyFields>;

} // namespace facebook::fboss
