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
constexpr auto kRules = "rules";
constexpr auto kName = "name";
} // namespace

namespace facebook {
namespace fboss {

folly::dynamic QosRule::toFollyDynamic() const {
  folly::dynamic qosRule = folly::dynamic::object;
  qosRule[kQueueId] = queueId;
  qosRule[kDscp] = dscp;
  return qosRule;
}

QosRule QosRule::fromFollyDynamic(const folly::dynamic& qosRuleJson) {
  if (qosRuleJson.find(kQueueId) == qosRuleJson.items().end()) {
    throw FbossError("QosRule must have a queueId set");
  }
  if (qosRuleJson.find(kDscp) == qosRuleJson.items().end()) {
    throw FbossError("QosRule must have a dscp value");
  }
  return QosRule(qosRuleJson[kQueueId].asInt(), qosRuleJson[kDscp].asInt());
}

folly::dynamic QosPolicyFields::toFollyDynamic() const {
  folly::dynamic qosPolicy = folly::dynamic::object;
  qosPolicy[kName] = name;
  folly::dynamic qosPolicys = folly::dynamic::array;
  for (const auto& rule : rules) {
    qosPolicys.push_back(rule.toFollyDynamic());
  }
  qosPolicy[kRules] = qosPolicys;
  return qosPolicy;
}

QosPolicyFields QosPolicyFields::fromFollyDynamic(const folly::dynamic& json) {
  auto name = json[kName].asString();
  std::set<QosRule> rules;
  for (const auto& rule : json[kRules]) {
    rules.emplace(QosRule::fromFollyDynamic(rule));
  }
  return QosPolicyFields(name, rules);
}

template class NodeBaseT<QosPolicy, QosPolicyFields>;

} // namespace fboss
} // namespace facebook
