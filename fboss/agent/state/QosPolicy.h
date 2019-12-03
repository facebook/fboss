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
#include "fboss/agent/state/NodeBase.h"
#include "fboss/agent/types.h"

#include <optional>
#include <string>
#include <utility>

#include <boost/container/flat_set.hpp>

namespace facebook {
namespace fboss {

struct QosRule {
  uint8_t queueId;
  uint8_t dscp;

  explicit QosRule(uint8_t queueId, uint8_t dscp)
      : queueId(queueId), dscp(dscp) {}

  QosRule(const QosRule& r) {
    queueId = r.queueId;
    dscp = r.dscp;
  }

  bool operator<(const QosRule& r) const {
    return std::tie(queueId, dscp) < std::tie(r.queueId, r.dscp);
  }

  bool operator==(const QosRule& r) const {
    return std::tie(queueId, dscp) == std::tie(r.queueId, r.dscp);
  }

  QosRule& operator=(const QosRule& r) {
    queueId = r.queueId;
    dscp = r.dscp;
    return *this;
  }

  folly::dynamic toFollyDynamic() const;
  static QosRule fromFollyDynamic(const folly::dynamic& rangeJson);
};

template <typename QosAttrT>
class TrafficClassToQosAttributeMapEntry {
 public:
  TrafficClassToQosAttributeMapEntry(TrafficClass trafficClass, QosAttrT attr)
      : trafficClass_(trafficClass), attr_(attr) {}
  TrafficClass trafficClass() {
    return trafficClass_;
  }
  QosAttrT attr() {
    return attr_;
  }

  bool operator<(const TrafficClassToQosAttributeMapEntry& other) const {
    return std::tie(trafficClass_, attr_) <
        std::tie(other.trafficClass_, other.attr_);
  }

  bool operator==(const TrafficClassToQosAttributeMapEntry& other) const {
    return std::tie(trafficClass_, attr_) ==
        std::tie(other.trafficClass_, other.attr_);
  }
  bool operator!=(const TrafficClassToQosAttributeMapEntry& other) const {
    return !(*this == other);
  }
  folly::dynamic toFollyDynamic() const;
  static TrafficClassToQosAttributeMapEntry<QosAttrT> fromFollyDynamic(
      folly::dynamic json);

 private:
  TrafficClass trafficClass_;
  QosAttrT attr_;
};

template <typename QosAttrT>
class TrafficClassToQosAttributeMap {
 public:
  using Entry = TrafficClassToQosAttributeMapEntry<QosAttrT>;
  void addFromEntry(TrafficClass trafficClass, QosAttrT attr) {
    from_.emplace(
        TrafficClassToQosAttributeMapEntry<QosAttrT>(trafficClass, attr));
  }
  void addToEntry(TrafficClass trafficClass, QosAttrT attr) {
    to_.emplace(
        TrafficClassToQosAttributeMapEntry<QosAttrT>(trafficClass, attr));
  }

  const boost::container::flat_set<Entry>& from() const {
    return from_;
  }

  const std::optional<Entry>& to() const {
    return to_;
  }

  bool operator<(const TrafficClassToQosAttributeMap& other) const {
    return std::tie(from_, to_) < std::tie(other.from_, other.to_);
  }

  bool operator==(const TrafficClassToQosAttributeMap& other) const {
    return std::tie(from_, to_) == std::tie(other.from_, other.to_);
  }
  bool operator!=(const TrafficClassToQosAttributeMap& other) const {
    return !(*this == other);
  }
  bool empty() const {
    return from_.empty() && !to_.has_value();
  }

  folly::dynamic toFollyDynamic() const;
  static TrafficClassToQosAttributeMap fromFollyDynamic(folly::dynamic json);

 private:
  boost::container::flat_set<Entry> from_;
  std::optional<Entry> to_;
};

class DscpMap : public TrafficClassToQosAttributeMap<DSCP> {
 public:
  explicit DscpMap(const TrafficClassToQosAttributeMap<DSCP>& map)
      : TrafficClassToQosAttributeMap<DSCP>(map) {}
  explicit DscpMap(std::vector<cfg::DscpQosMap> cfg);
};

class ExpMap : public TrafficClassToQosAttributeMap<EXP> {
 public:
  explicit ExpMap(const TrafficClassToQosAttributeMap<EXP>& map)
      : TrafficClassToQosAttributeMap<EXP>(map) {}
  explicit ExpMap(std::vector<cfg::ExpQosMap> cfg);
};

struct QosPolicyFields {
  using TrafficClassToQueueId =
      boost::container::flat_map<TrafficClass, uint16_t>;
  QosPolicyFields(
      const std::string& name,
      const std::set<QosRule>& rules,
      DscpMap dscpMap,
      ExpMap expMap,
      TrafficClassToQueueId trafficClassToQueueId)
      : name(name),
        rules(rules),
        dscpMap(std::move(dscpMap)),
        expMap(std::move(expMap)),
        trafficClassToQueueId(std::move(trafficClassToQueueId)) {}

  QosPolicyFields(const std::string& name, const std::set<QosRule>& rules)
      : QosPolicyFields(
            name,
            rules,
            DscpMap(std::vector<cfg::DscpQosMap>{}),
            ExpMap(std::vector<cfg::ExpQosMap>{}),
            TrafficClassToQueueId()) {
    /* TODO(pshaikh): remove this constructor */
  }

  template <typename Fn>
  void forEachChild(Fn) {}

  folly::dynamic toFollyDynamic() const;
  static QosPolicyFields fromFollyDynamic(const folly::dynamic& json);

  std::string name{nullptr};
  std::set<QosRule> rules;
  DscpMap dscpMap;
  ExpMap expMap;
  TrafficClassToQueueId trafficClassToQueueId;
};

class QosPolicy : public NodeBaseT<QosPolicy, QosPolicyFields> {
 public:
  using TrafficClassToQueueId = QosPolicyFields::TrafficClassToQueueId;
  QosPolicy(
      const std::string& name,
      const std::set<QosRule>& rules,
      // TODO(pshaikh) : remove default argument settings
      DscpMap dscpMap = DscpMap(std::vector<cfg::DscpQosMap>{}),
      ExpMap expMap = ExpMap(std::vector<cfg::ExpQosMap>{}),
      TrafficClassToQueueId trafficClassToQueueId = TrafficClassToQueueId())
      : NodeBaseT(name, rules, dscpMap, expMap, trafficClassToQueueId) {}

  static std::shared_ptr<QosPolicy> fromFollyDynamic(
      const folly::dynamic& json) {
    const auto& fields = QosPolicyFields::fromFollyDynamic(json);
    return std::make_shared<QosPolicy>(fields);
  }

  static std::shared_ptr<QosPolicy> fromJson(const folly::fbstring& jsonStr) {
    return fromFollyDynamic(folly::parseJson(jsonStr));
  }

  folly::dynamic toFollyDynamic() const override {
    return getFields()->toFollyDynamic();
  }

  bool operator==(const QosPolicy& qosPolicy) const {
    return getFields()->name == qosPolicy.getName() &&
        getFields()->rules == qosPolicy.getRules() &&
        getFields()->dscpMap == qosPolicy.getDscpMap() &&
        getFields()->expMap == qosPolicy.getExpMap() &&
        getFields()->trafficClassToQueueId ==
        qosPolicy.getTrafficClassToQueueId();
  }

  const std::string& getName() const {
    return getFields()->name;
  }

  const std::string& getID() const {
    return getName();
  }

  const std::set<QosRule>& getRules() const {
    return getFields()->rules;
  }

  void setQosRules(const std::set<QosRule>& qosRules) {
    writableFields()->rules = qosRules;
  }

  const DscpMap& getDscpMap() const {
    return getFields()->dscpMap;
  }

  const ExpMap& getExpMap() const {
    return getFields()->expMap;
  }

  const TrafficClassToQueueId& getTrafficClassToQueueId() const {
    return getFields()->trafficClassToQueueId;
  }

 private:
  // Inherit the constructors required for clone()
  using NodeBaseT::NodeBaseT;
  friend class CloneAllocator;
};

} // namespace fboss
} // namespace facebook
