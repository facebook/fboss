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

template <typename EntryT>
const folly::dynamic QosPolicyFields::entryToFolly(
    state::TrafficClassToQosAttributeEntry entry) const {
  folly::dynamic follyEntry = folly::dynamic::object;
  follyEntry[kTrafficClass] = folly::to<std::string>(*entry.trafficClass());
  if (std::is_same<EntryT, DSCP>::value) {
    follyEntry[kDscp] = folly::to<std::string>(*entry.attr());
  } else {
    follyEntry[kExp] = folly::to<std::string>(*entry.attr());
  }
  return follyEntry;
}

template <typename EntryT>
const folly::dynamic QosPolicyFields::entryListToFolly(
    std::vector<state::TrafficClassToQosAttributeEntry> list) const {
  folly::dynamic follyList = folly::dynamic::array;
  for (const auto& dscpEntry : list) {
    follyList.push_back(entryToFolly<EntryT>(dscpEntry));
  }
  return follyList;
}

template <typename EntryT>
state::TrafficClassToQosAttributeEntry QosPolicyFields::entryFromFolly(
    folly::dynamic json) {
  auto attrStr = kDscp;
  if (std::is_same<EntryT, EXP>::value) {
    attrStr = kExp;
  }
  if (json.find(kTrafficClass) == json.items().end() ||
      json.find(attrStr) == json.items().end()) {
    throw FbossError(folly::to<std::string>(
        "Cannot find ", kTrafficClass, "or ", attrStr, " in dscp/exp entry."));
  }

  state::TrafficClassToQosAttributeEntry entry;
  entry.trafficClass() = json[kTrafficClass].asInt();
  entry.attr() = json[attrStr].asInt();
  return entry;
}

template <typename EntryT>
std::vector<state::TrafficClassToQosAttributeEntry>
QosPolicyFields::entryListFromFolly(folly::dynamic json) {
  std::vector<state::TrafficClassToQosAttributeEntry> entryList;
  for (const auto& entry : json) {
    entryList.push_back(entryFromFolly<EntryT>(entry));
  }
  return entryList;
}

QosPolicyFields QosPolicyFields::fromThrift(
    state::QosPolicyFields const& qosPolicyFields) {
  auto fields = QosPolicyFields(
      *qosPolicyFields.name(),
      DscpMap(*qosPolicyFields.dscpMap()),
      ExpMap(*qosPolicyFields.expMap()),
      *qosPolicyFields.trafficClassToQueueId());
  fields.writableData() = qosPolicyFields;
  return fields;
}

folly::dynamic QosPolicyFields::migrateToThrifty(folly::dynamic const& dyn) {
  folly::dynamic newDyn = dyn;
  // Dscp Map
  if (newDyn.find(kDscpMap) != newDyn.items().end()) {
    auto& dscpMapFolly = newDyn[kDscpMap];
    if (dscpMapFolly.find(kFrom) != newDyn.items().end()) {
      auto& dscpMapFrom = dscpMapFolly[kFrom];
      for (auto& entry : dscpMapFrom) {
        ThriftyUtils::stringToInt(entry, kTrafficClass);
        ThriftyUtils::stringToInt(entry, kDscp);
        ThriftyUtils::renameField(entry, kDscp, kAttr);
      }
    }
    if (dscpMapFolly.find(kTo) != newDyn.items().end()) {
      auto& dscpMapTo = dscpMapFolly[kTo];
      for (auto& entry : dscpMapTo) {
        ThriftyUtils::stringToInt(entry, kTrafficClass);
        ThriftyUtils::stringToInt(entry, kDscp);
        ThriftyUtils::renameField(entry, kDscp, kAttr);
      }
    }
  }
  // Exp Map
  if (newDyn.find(kExpMap) != newDyn.items().end()) {
    auto& expMapFolly = newDyn[kExpMap];
    if (expMapFolly.find(kFrom) != newDyn.items().end()) {
      auto& expMapFrom = expMapFolly[kFrom];
      for (auto& entry : expMapFrom) {
        ThriftyUtils::stringToInt(entry, kTrafficClass);
        ThriftyUtils::stringToInt(entry, kExp);
        ThriftyUtils::renameField(entry, kExp, kAttr);
      }
    }
    if (expMapFolly.find(kTo) != newDyn.items().end()) {
      auto& expMapTo = expMapFolly[kTo];
      for (auto& entry : expMapTo) {
        ThriftyUtils::stringToInt(entry, kTrafficClass);
        ThriftyUtils::stringToInt(entry, kExp);
        ThriftyUtils::renameField(entry, kExp, kAttr);
      }
    }
  }
  ThriftyUtils::follyArraytoThriftyMap(
      newDyn, kTrafficClassToQueueId, kTrafficClass, kQueueId);
  ThriftyUtils::follyArraytoThriftyMap(
      newDyn, kPfcPriorityToQueueId, kPfcPriority, kQueueId);
  ThriftyUtils::follyArraytoThriftyMap(
      newDyn, kTrafficClassToPgId, kTrafficClass, kPgId);
  ThriftyUtils::follyArraytoThriftyMap(
      newDyn, kPfcPriorityToPgId, kPfcPriority, kPgId);
  return newDyn;
}

void QosPolicyFields::migrateFromThrifty(folly::dynamic& dyn) {
  // Dscp Map
  if (dyn.find(kDscpMap) != dyn.items().end()) {
    auto& dscpMapFolly = dyn[kDscpMap];
    if (dscpMapFolly.find(kFrom) != dyn.items().end()) {
      auto& dscpMapFrom = dscpMapFolly[kFrom];
      for (auto& entry : dscpMapFrom) {
        ThriftyUtils::renameField(entry, kAttr, kDscp);
      }
    }
    if (dscpMapFolly.find(kTo) != dyn.items().end()) {
      auto& dscpMapTo = dscpMapFolly[kTo];
      for (auto& entry : dscpMapTo) {
        ThriftyUtils::renameField(entry, kAttr, kDscp);
      }
    }
  }

  // Exp Map
  if (dyn.find(kExpMap) != dyn.items().end()) {
    auto& expMapFolly = dyn[kExpMap];
    if (expMapFolly.find(kFrom) != dyn.items().end()) {
      auto& expMapFrom = expMapFolly[kFrom];
      for (auto& entry : expMapFrom) {
        ThriftyUtils::renameField(entry, kAttr, kExp);
      }
    }
    if (expMapFolly.find(kTo) != dyn.items().end()) {
      auto& expMapTo = expMapFolly[kTo];
      for (auto& entry : expMapTo) {
        ThriftyUtils::renameField(entry, kAttr, kExp);
      }
    }
  }

  ThriftyUtils::thriftyMapToFollyArray(
      dyn, kTrafficClassToQueueId, kTrafficClass, kQueueId);
  ThriftyUtils::thriftyMapToFollyArray(
      dyn, kPfcPriorityToQueueId, kPfcPriority, kQueueId);
  ThriftyUtils::thriftyMapToFollyArray(
      dyn, kTrafficClassToPgId, kTrafficClass, kPgId);
  ThriftyUtils::thriftyMapToFollyArray(
      dyn, kPfcPriorityToPgId, kPfcPriority, kPgId);
}

folly::dynamic QosPolicyFields::toFollyDynamicLegacy() const {
  folly::dynamic qosPolicy = folly::dynamic::object;
  qosPolicy[kName] = *data().name();

  // Dscp Map
  folly::dynamic dscpMap = folly::dynamic::object;
  dscpMap[kFrom] = entryListToFolly<DSCP>(*data().dscpMap()->from());
  dscpMap[kTo] = entryListToFolly<DSCP>(*data().dscpMap()->to());
  qosPolicy[kDscpMap] = dscpMap;

  // Exp Map
  folly::dynamic expMap = folly::dynamic::object;
  expMap[kFrom] = entryListToFolly<EXP>(*data().expMap()->from());
  expMap[kTo] = entryListToFolly<EXP>(*data().expMap()->to());
  qosPolicy[kExpMap] = expMap;

  // TrafficClassToQueueId

  qosPolicy[kTrafficClassToQueueId] = folly::dynamic::array;
  // TODO(pshaikh): remove below after one push
  qosPolicy[kRules] = folly::dynamic::array;
  for (auto entry : *data().trafficClassToQueueId()) {
    folly::dynamic jsonEntry = folly::dynamic::object;
    jsonEntry[kTrafficClass] = entry.first;
    jsonEntry[kQueueId] = entry.second;
    qosPolicy[kTrafficClassToQueueId].push_back(jsonEntry);
  }
  if (auto pfcPri2QueueId = data().pfcPriorityToQueueId().to_optional()) {
    qosPolicy[kPfcPriorityToQueueId] = folly::dynamic::array;
    for (const auto& pfcPri : *pfcPri2QueueId) {
      folly::dynamic jsonEntry = folly::dynamic::object;
      jsonEntry[kPfcPriority] = pfcPri.first;
      jsonEntry[kQueueId] = pfcPri.second;
      qosPolicy[kPfcPriorityToQueueId].push_back(jsonEntry);
    }
  }
  if (auto trafficClass2PgId = data().trafficClassToPgId().to_optional()) {
    qosPolicy[kTrafficClassToPgId] = folly::dynamic::array;
    for (const auto& tc2PgId : *trafficClass2PgId) {
      folly::dynamic jsonEntry = folly::dynamic::object;
      jsonEntry[kTrafficClass] = tc2PgId.first;
      jsonEntry[kPgId] = tc2PgId.second;
      qosPolicy[kTrafficClassToPgId].push_back(jsonEntry);
    }
  }
  if (auto pfcPriority2PgId = data().pfcPriorityToPgId().to_optional()) {
    qosPolicy[kPfcPriorityToPgId] = folly::dynamic::array;
    for (const auto& pfcPri2PgId : *pfcPriority2PgId) {
      folly::dynamic jsonEntry = folly::dynamic::object;
      jsonEntry[kPfcPriority] = pfcPri2PgId.first;
      jsonEntry[kPgId] = pfcPri2PgId.second;
      qosPolicy[kPfcPriorityToPgId].push_back(jsonEntry);
    }
  }
  return qosPolicy;
}

QosPolicyFields QosPolicyFields::fromFollyDynamicLegacy(
    const folly::dynamic& json) {
  auto name = json[kName].asString();

  // Dscp Map
  DscpMap dscpMap;
  if (json.find(kDscpMap) != json.items().end()) {
    state::TrafficClassToQosAttributeMap dscpMapThrift;
    const auto& dscpMapFolly = json[kDscpMap];

    if (dscpMapFolly.find(kFrom) != json.items().end()) {
      dscpMapThrift.from() = entryListFromFolly<DSCP>(dscpMapFolly[kFrom]);
    }
    if (dscpMapFolly.find(kTo) != json.items().end()) {
      dscpMapThrift.to() = entryListFromFolly<DSCP>(dscpMapFolly[kTo]);
    }
    dscpMap = DscpMap(dscpMapThrift);
  }

  // Exp Map
  ExpMap expMap;
  if (json.find(kExpMap) != json.items().end()) {
    state::TrafficClassToQosAttributeMap expMapThrift;
    const auto& expMapFolly = json[kExpMap];

    if (expMapFolly.find(kFrom) != json.items().end()) {
      expMapThrift.from() = entryListFromFolly<EXP>(expMapFolly[kFrom]);
    }
    if (expMapFolly.find(kTo) != json.items().end()) {
      expMapThrift.to() = entryListFromFolly<EXP>(expMapFolly[kTo]);
    }
    expMap = ExpMap(expMapThrift);
  }

  // trafficClassToQueueId
  std::map<int16_t, int16_t> trafficClassToQueueId;
  if (json.find(kTrafficClassToQueueId) != json.items().end()) {
    for (const auto& entry : json[kTrafficClassToQueueId]) {
      trafficClassToQueueId.emplace(
          entry[kTrafficClass].asInt(), entry[kQueueId].asInt());
    }
  }

  auto qosPolicyFields =
      QosPolicyFields(name, dscpMap, expMap, trafficClassToQueueId);

  // pfcPriorityToQueueId
  if (json.find(kPfcPriorityToQueueId) != json.items().end()) {
    std::map<int16_t, int16_t> pfcPriorityToQueueId;
    for (const auto& pfcPriQueueIdEntry : json[kPfcPriorityToQueueId]) {
      pfcPriorityToQueueId.emplace(
          pfcPriQueueIdEntry[kPfcPriority].asInt(),
          pfcPriQueueIdEntry[kQueueId].asInt());
    }
    qosPolicyFields.writableData().pfcPriorityToQueueId() =
        pfcPriorityToQueueId;
  }

  // trafficClassToPgId
  if (json.find(kTrafficClassToPgId) != json.items().end()) {
    std::map<int16_t, int16_t> trafficClassToPgId;
    for (const auto& tc2PgId : json[kTrafficClassToPgId]) {
      trafficClassToPgId.emplace(
          tc2PgId[kTrafficClass].asInt(), tc2PgId[kPgId].asInt());
    }
    qosPolicyFields.writableData().trafficClassToPgId() = trafficClassToPgId;
  }

  // pfcPriorityToPgId
  if (json.find(kPfcPriorityToPgId) != json.items().end()) {
    std::map<int16_t, int16_t> pfcPriorityToPgId;
    for (const auto& pfcPri2PgId : json[kPfcPriorityToPgId]) {
      pfcPriorityToPgId.emplace(
          static_cast<PfcPriority>(pfcPri2PgId[kPfcPriority].asInt()),
          pfcPri2PgId[kPgId].asInt());
    }
    qosPolicyFields.writableData().pfcPriorityToPgId() = pfcPriorityToPgId;
  }
  return qosPolicyFields;
}

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

template class ThriftyBaseT<state::QosPolicyFields, QosPolicy, QosPolicyFields>;

} // namespace facebook::fboss
