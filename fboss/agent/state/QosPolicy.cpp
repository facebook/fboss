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
constexpr auto kFrom = "from";
constexpr auto kTo = "to";
} // namespace

namespace facebook::fboss {

folly::dynamic QosPolicyFields::toFollyDynamic() const {
  folly::dynamic qosPolicy = folly::dynamic::object;
  qosPolicy[kName] = name;
  qosPolicy[kDscpMap] = dscpMap.toFollyDynamic();
  qosPolicy[kExpMap] = expMap.toFollyDynamic();
  qosPolicy[kTrafficClassToQueueId] = folly::dynamic::array;
  // TODO(pshaikh): remove below after one push
  qosPolicy[kRules] = folly::dynamic::array;
  for (auto entry : trafficClassToQueueId) {
    folly::dynamic jsonEntry = folly::dynamic::object;
    jsonEntry[kTrafficClass] = static_cast<uint16_t>(entry.first);
    jsonEntry[kQueueId] = entry.second;
    qosPolicy[kTrafficClassToQueueId].push_back(jsonEntry);
  }
  return qosPolicy;
}

QosPolicyFields QosPolicyFields::fromFollyDynamic(const folly::dynamic& json) {
  auto name = json[kName].asString();
  DscpMap ingressDscpMap;
  TrafficClassToQosAttributeMap<DSCP> dscpMap;
  TrafficClassToQosAttributeMap<EXP> expMap;
  TrafficClassToQueueId trafficClassToQueueId;

  if (json.find(kDscpMap) != json.items().end()) {
    dscpMap =
        TrafficClassToQosAttributeMap<DSCP>::fromFollyDynamic(json[kDscpMap]);
    ingressDscpMap = DscpMap(dscpMap);
  }
  if (json.find(kExpMap) != json.items().end()) {
    expMap =
        TrafficClassToQosAttributeMap<EXP>::fromFollyDynamic(json[kExpMap]);
  }
  if (json.find(kTrafficClassToQueueId) != json.items().end()) {
    for (const auto& entry : json[kTrafficClassToQueueId]) {
      trafficClassToQueueId.emplace(
          static_cast<TrafficClass>(entry[kTrafficClass].asInt()),
          entry[kQueueId].asInt());
    }
  }
  return QosPolicyFields(
      name, ingressDscpMap, ExpMap(expMap), trafficClassToQueueId);
}

DscpMap::DscpMap(std::vector<cfg::DscpQosMap> cfg)
    : TrafficClassToQosAttributeMap<DSCP>() {
  for (auto map : cfg) {
    auto trafficClass = *map.internalTrafficClass_ref();

    for (auto dscp : *map.fromDscpToTrafficClass_ref()) {
      addFromEntry(
          static_cast<TrafficClass>(trafficClass), static_cast<DSCP>(dscp));
    }
    if (auto fromTrafficClassToDscp = map.fromTrafficClassToDscp_ref()) {
      addToEntry(
          static_cast<TrafficClass>(trafficClass),
          static_cast<DSCP>(fromTrafficClassToDscp.value()));
    }
  }
}

ExpMap::ExpMap(std::vector<cfg::ExpQosMap> cfg)
    : TrafficClassToQosAttributeMap<EXP>() {
  for (auto map : cfg) {
    auto trafficClass = *map.internalTrafficClass_ref();

    for (auto dscp : *map.fromExpToTrafficClass_ref()) {
      addFromEntry(
          static_cast<TrafficClass>(trafficClass), static_cast<EXP>(dscp));
    }
    if (auto fromTrafficClassToExp = map.fromTrafficClassToExp_ref()) {
      addToEntry(
          static_cast<TrafficClass>(trafficClass),
          static_cast<EXP>(fromTrafficClassToExp.value()));
    }
  }
}

template <>
folly::dynamic TrafficClassToQosAttributeMapEntry<DSCP>::toFollyDynamic()
    const {
  folly::dynamic object = folly::dynamic::object;
  object[kTrafficClass] = folly::to<std::string>(trafficClass_);
  object[kDscp] = folly::to<std::string>(attr_);
  return object;
}

template <>
folly::dynamic TrafficClassToQosAttributeMapEntry<EXP>::toFollyDynamic() const {
  folly::dynamic object = folly::dynamic::object;
  object[kTrafficClass] = folly::to<std::string>(trafficClass_);
  object[kExp] = folly::to<std::string>(attr_);
  return object;
}

template <>
TrafficClassToQosAttributeMapEntry<DSCP>
TrafficClassToQosAttributeMapEntry<DSCP>::fromFollyDynamic(
    folly::dynamic json) {
  if (json.find(kTrafficClass) == json.items().end() ||
      json.find(kDscp) == json.items().end()) {
    throw FbossError("");
  }

  return TrafficClassToQosAttributeMapEntry<DSCP>(
      static_cast<TrafficClass>(json[kTrafficClass].asInt()),
      static_cast<DSCP>(json[kDscp].asInt()));
}

template <>
TrafficClassToQosAttributeMapEntry<EXP>
TrafficClassToQosAttributeMapEntry<EXP>::fromFollyDynamic(folly::dynamic json) {
  if (json.find(kTrafficClass) == json.items().end() ||
      json.find(kExp) == json.items().end()) {
    throw FbossError("");
  }

  return TrafficClassToQosAttributeMapEntry<EXP>(
      static_cast<TrafficClass>(json[kTrafficClass].asInt()),
      static_cast<EXP>(json[kExp].asInt()));
}

template <typename QosAttrT>
folly::dynamic TrafficClassToQosAttributeMap<QosAttrT>::toFollyDynamic() const {
  folly::dynamic object = folly::dynamic::object;
  folly::dynamic fromEntries = folly::dynamic::array;
  for (const auto& entry : from_) {
    fromEntries.push_back(entry.toFollyDynamic());
  }
  object[kFrom] = fromEntries;
  folly::dynamic toEntries = folly::dynamic::array;
  for (const auto& entry : to_) {
    toEntries.push_back(entry.toFollyDynamic());
  }
  object[kTo] = toEntries;
  return object;
}

template <typename QosAttrT>
TrafficClassToQosAttributeMap<QosAttrT>
TrafficClassToQosAttributeMap<QosAttrT>::fromFollyDynamic(folly::dynamic json) {
  TrafficClassToQosAttributeMap<QosAttrT> map;
  for (const auto& entry : json[kFrom]) {
    map.from_.insert(
        TrafficClassToQosAttributeMapEntry<QosAttrT>::fromFollyDynamic(entry));
  }
  if (json.find(kTo) != json.items().end()) {
    // TODO: remove this check after another push
    for (const auto& entry : json[kTo]) {
      map.to_.insert(
          TrafficClassToQosAttributeMapEntry<QosAttrT>::fromFollyDynamic(
              entry));
    }
  }
  return map;
}

template class TrafficClassToQosAttributeMap<DSCP>;
template class TrafficClassToQosAttributeMap<EXP>;
template class NodeBaseT<QosPolicy, QosPolicyFields>;

} // namespace facebook::fboss
