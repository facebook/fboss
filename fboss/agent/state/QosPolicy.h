/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include "fboss/agent/FbossError.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/gen-cpp2/switch_state_types.h"
#include "fboss/agent/state/Thrifty.h"
#include "fboss/agent/types.h"

#include <optional>
#include <string>
#include <utility>

namespace facebook::fboss {

template <typename QosAttrT>
class TrafficClassToQosAttributeMap
    : public BetterThriftyFields<
          TrafficClassToQosAttributeMap<QosAttrT>,
          state::TrafficClassToQosAttributeMap> {
 public:
  TrafficClassToQosAttributeMap() {}
  explicit TrafficClassToQosAttributeMap(
      const state::TrafficClassToQosAttributeMap& map) {
    this->data = map;
  }

  void addFromEntry(TrafficClass trafficClass, QosAttrT attr) {
    state::TrafficClassToQosAttributeEntry entry =
        state::TrafficClassToQosAttributeEntry();
    entry.trafficClass() = trafficClass;
    entry.attr() = attr;
    this->data.from()->push_back(entry);
  }
  void addToEntry(TrafficClass trafficClass, QosAttrT attr) {
    state::TrafficClassToQosAttributeEntry entry =
        state::TrafficClassToQosAttributeEntry();
    entry.trafficClass() = trafficClass;
    entry.attr() = attr;
    this->data.to()->push_back(entry);
  }

  const std::vector<state::TrafficClassToQosAttributeEntry> from() const {
    return *this->data.from();
  }

  const std::vector<state::TrafficClassToQosAttributeEntry> to() const {
    return *this->data.to();
  }

  bool empty() const {
    return (*this->data.from()).empty() && (*this->data.to()).empty();
  }
};

class ExpMap : public TrafficClassToQosAttributeMap<EXP> {
 public:
  ExpMap() {}
  explicit ExpMap(std::vector<cfg::ExpQosMap> cfg);
  explicit ExpMap(const state::TrafficClassToQosAttributeMap& map)
      : TrafficClassToQosAttributeMap(map) {}
};

class DscpMap : public TrafficClassToQosAttributeMap<DSCP> {
 public:
  DscpMap() {}
  explicit DscpMap(std::vector<cfg::DscpQosMap> cfg);
  explicit DscpMap(const state::TrafficClassToQosAttributeMap& map)
      : TrafficClassToQosAttributeMap(map) {}
};

struct QosPolicyFields
    : public BetterThriftyFields<QosPolicyFields, state::QosPolicyFields> {
  QosPolicyFields(
      const std::string& name,
      DscpMap dscpMap,
      ExpMap expMap,
      std::map<int16_t, int16_t> trafficClassToQueueId) {
    *data.name() = name;
    *data.dscpMap() = dscpMap.data;
    *data.expMap() = expMap.data;
    *data.trafficClassToQueueId() = trafficClassToQueueId;
  }

  template <typename Fn>
  void forEachChild(Fn) {}

  template <typename EntryT>
  const folly::dynamic entryToFolly(
      state::TrafficClassToQosAttributeEntry entry) const;

  template <typename EntryT>
  const folly::dynamic entryListToFolly(
      std::vector<state::TrafficClassToQosAttributeEntry> set) const;

  template <typename EntryT>
  static state::TrafficClassToQosAttributeEntry entryFromFolly(
      folly::dynamic entry);

  template <typename EntryT>
  static std::vector<state::TrafficClassToQosAttributeEntry> entryListFromFolly(
      folly::dynamic entry);

  static QosPolicyFields fromThrift(
      state::QosPolicyFields const& qosPolicyFields);
  static folly::dynamic migrateToThrifty(folly::dynamic const& dyn);
  static void migrateFromThrifty(folly::dynamic& dyn);
  folly::dynamic toFollyDynamicLegacy() const;
  static QosPolicyFields fromFollyDynamicLegacy(
      const folly::dynamic& qosPolicy);
};

class QosPolicy
    : public ThriftyBaseT<state::QosPolicyFields, QosPolicy, QosPolicyFields> {
 public:
  QosPolicy(
      const std::string& name,
      DscpMap dscpMap,
      ExpMap expMap = ExpMap(std::vector<cfg::ExpQosMap>{}),
      std::map<int16_t, int16_t> trafficClassToQueueId = {})
      : ThriftyBaseT(name, dscpMap, expMap, trafficClassToQueueId) {}

  bool operator!=(const QosPolicy& qosPolicy) const {
    return !(*this == qosPolicy);
  }

  const std::string& getName() const {
    return *getFields()->data.name();
  }

  const std::string& getID() const {
    return getName();
  }

  const DscpMap getDscpMap() const {
    return DscpMap(*getFields()->data.dscpMap());
  }

  void setDscpMap(DscpMap dscpMap) {
    writableFields()->data.dscpMap() = dscpMap.data;
  }

  const ExpMap getExpMap() const {
    return ExpMap(*getFields()->data.expMap());
  }

  const std::map<int16_t, int16_t> getTrafficClassToQueueId() const {
    return *getFields()->data.trafficClassToQueueId();
  }

  std::optional<std::map<int16_t, int16_t>> getPfcPriorityToQueueId() {
    return getFields()->data.pfcPriorityToQueueId().to_optional();
  }

  std::optional<std::map<int16_t, int16_t>> getTrafficClassToPgId() {
    return getFields()->data.trafficClassToPgId().to_optional();
  }

  std::optional<std::map<int16_t, int16_t>> getPfcPriorityToPgId() {
    return getFields()->data.pfcPriorityToPgId().to_optional();
  }

  void setExpMap(ExpMap expMap) {
    writableFields()->data.expMap() = expMap.data;
  }

  void setTrafficClassToQueueIdMap(const std::map<int16_t, int16_t>& tc2Q) {
    writableFields()->data.trafficClassToQueueId() = tc2Q;
  }

  void setPfcPriorityToQueueIdMap(
      const std::map<int16_t, int16_t>& pfcPri2QueueId) {
    writableFields()->data.pfcPriorityToQueueId() = pfcPri2QueueId;
  }

  void setTrafficClassToPgIdMap(
      const std::map<int16_t, int16_t>& trafficClass2PgId) {
    writableFields()->data.trafficClassToPgId() = trafficClass2PgId;
  }

  void setPfcPriorityToPgIdMap(
      const std::map<int16_t, int16_t>& pfcPriority2PgId) {
    writableFields()->data.pfcPriorityToPgId() = pfcPriority2PgId;
  }

 private:
  // Inherit the constructors required for clone()
  using ThriftyBaseT<state::QosPolicyFields, QosPolicy, QosPolicyFields>::
      ThriftyBaseT;
  friend class CloneAllocator;
};

} // namespace facebook::fboss
