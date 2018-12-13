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

#include <folly/Optional.h>
#include <string>
#include <utility>

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

struct QosPolicyFields {
  explicit QosPolicyFields(const std::string& name, std::set<QosRule>& rules)
      : name(name), rules(rules) {}

  template <typename Fn>
  void forEachChild(Fn) {}

  folly::dynamic toFollyDynamic() const;
  static QosPolicyFields fromFollyDynamic(const folly::dynamic& json);

  std::string name{nullptr};
  std::set<QosRule> rules;
};

class QosPolicy : public NodeBaseT<QosPolicy, QosPolicyFields> {
 public:
  explicit QosPolicy(const std::string& name, std::set<QosRule>& rules)
      : NodeBaseT(name, rules) {}

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

  bool operator==(const QosPolicy& qosPolicy) {
    return getFields()->name == qosPolicy.getName() &&
        getFields()->rules == qosPolicy.getRules();
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

 private:
  // Inherit the constructors required for clone()
  using NodeBaseT::NodeBaseT;
  friend class CloneAllocator;
};

} // namespace fboss
} // namespace facebook
