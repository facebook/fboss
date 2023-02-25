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
class TrafficClassToQosAttributeMap {
 public:
  TrafficClassToQosAttributeMap() {}
  explicit TrafficClassToQosAttributeMap(
      const state::TrafficClassToQosAttributeMap& map) {
    this->writableData() = map;
  }

  void addFromEntry(TrafficClass trafficClass, QosAttrT attr) {
    state::TrafficClassToQosAttributeEntry entry =
        state::TrafficClassToQosAttributeEntry();
    entry.trafficClass() = trafficClass;
    entry.attr() = attr;
    this->writableData().from()->push_back(entry);
  }
  void addToEntry(TrafficClass trafficClass, QosAttrT attr) {
    state::TrafficClassToQosAttributeEntry entry =
        state::TrafficClassToQosAttributeEntry();
    entry.trafficClass() = trafficClass;
    entry.attr() = attr;
    this->writableData().to()->push_back(entry);
  }

  const std::vector<state::TrafficClassToQosAttributeEntry> from() const {
    return *this->data().from();
  }

  const std::vector<state::TrafficClassToQosAttributeEntry> to() const {
    return *this->data().to();
  }

  bool empty() const {
    return this->data().from()->empty() && this->data().to()->empty();
  }

  state::TrafficClassToQosAttributeMap toThrift() const {
    return this->data();
  }

  static TrafficClassToQosAttributeMap<QosAttrT> fromThrift(
      const state::TrafficClassToQosAttributeMap& systemPortThrift) {
    return TrafficClassToQosAttributeMap<QosAttrT>(systemPortThrift);
  }

  bool operator==(const TrafficClassToQosAttributeMap& other) const {
    return this->data() == other.data();
  }

  const state::TrafficClassToQosAttributeMap& data() const {
    return data_;
  }

 private:
  state::TrafficClassToQosAttributeMap& writableData() {
    return data_;
  }
  state::TrafficClassToQosAttributeMap data_{};
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

USE_THRIFT_COW(QosPolicy)

class QosPolicy : public ThriftStructNode<QosPolicy, state::QosPolicyFields> {
 public:
  using Base = ThriftStructNode<QosPolicy, state::QosPolicyFields>;
  QosPolicy(
      const std::string& name,
      DscpMap dscpMap,
      ExpMap expMap = ExpMap(std::vector<cfg::ExpQosMap>{}),
      std::map<int16_t, int16_t> trafficClassToQueueId = {}) {
    set<switch_state_tags::name>(name);
    setDscpMap(std::move(dscpMap));
    setExpMap(std::move(expMap));
    setTrafficClassToQueueIdMap(std::move(trafficClassToQueueId));
  }

  const std::string& getName() const {
    return getID();
  }

  const std::string& getID() const {
    return cref<switch_state_tags::name>()->cref();
  }

  const auto& getDscpMap() const {
    return cref<switch_state_tags::dscpMap>();
  }

  void setDscpMap(DscpMap dscpMap) {
    set<switch_state_tags::dscpMap>(dscpMap.data());
  }

  const auto& getExpMap() const {
    return cref<switch_state_tags::expMap>();
  }

  const auto& getTrafficClassToQueueId() const {
    return cref<switch_state_tags::trafficClassToQueueId>();
  }

  const auto& getPfcPriorityToQueueId() const {
    return cref<switch_state_tags::pfcPriorityToQueueId>();
  }

  const auto& getTrafficClassToPgId() {
    return cref<switch_state_tags::trafficClassToPgId>();
  }

  const auto& getPfcPriorityToPgId() {
    return cref<switch_state_tags::pfcPriorityToPgId>();
  }

  void setExpMap(ExpMap expMap) {
    set<switch_state_tags::expMap>(expMap.data());
  }

  void setTrafficClassToQueueIdMap(const std::map<int16_t, int16_t>& tc2Q) {
    set<switch_state_tags::trafficClassToQueueId>(tc2Q);
  }

  void setPfcPriorityToQueueIdMap(
      const std::map<int16_t, int16_t>& pfcPri2QueueId) {
    set<switch_state_tags::pfcPriorityToQueueId>(pfcPri2QueueId);
  }

  void setTrafficClassToPgIdMap(
      const std::map<int16_t, int16_t>& trafficClass2PgId) {
    set<switch_state_tags::trafficClassToPgId>(trafficClass2PgId);
  }

  void setPfcPriorityToPgIdMap(
      const std::map<int16_t, int16_t>& pfcPriority2PgId) {
    set<switch_state_tags::pfcPriorityToPgId>(pfcPriority2PgId);
  }

 private:
  // Inherit the constructors required for clone()
  using Base::Base;
  friend class CloneAllocator;
};

} // namespace facebook::fboss
