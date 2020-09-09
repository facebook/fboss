/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/test/TrafficPolicyUtils.h"

#include "fboss/agent/hw/test/ConfigFactory.h"

namespace facebook::fboss::utility {
void addDscpAclToCfg(
    cfg::SwitchConfig* config,
    const std::string& aclName,
    uint32_t dscp) {
  auto numCfgAcls = config->acls_ref()->size();
  config->acls_ref()->resize(numCfgAcls + 1);
  *config->acls_ref()[numCfgAcls].name_ref() = aclName;
  config->acls_ref()[numCfgAcls].dscp_ref() = dscp;
}

void addClassIDAclToCfg(
    cfg::SwitchConfig* config,
    const std::string& aclName,
    cfg::AclLookupClass lookupClass) {
  auto numCfgAcls = config->acls_ref()->size();
  config->acls_ref()->resize(numCfgAcls + 1);
  *config->acls_ref()[numCfgAcls].name_ref() = aclName;
  config->acls_ref()[numCfgAcls].lookupClass_ref() = lookupClass;
}

void addQueueMatcher(
    cfg::SwitchConfig* config,
    const std::string& matcherName,
    int queueId,
    const std::optional<std::string>& counterName) {
  cfg::QueueMatchAction queueAction;
  *queueAction.queueId_ref() = queueId;
  cfg::MatchAction matchAction = cfg::MatchAction();
  matchAction.sendToQueue_ref() = queueAction;

  if (counterName.has_value()) {
    matchAction.counter_ref() = counterName.value();
  }

  utility::addMatcher(config, matcherName, matchAction);
}

void addTrafficCounter(
    cfg::SwitchConfig* config,
    const std::string& counterName) {
  auto counter = cfg::TrafficCounter();
  *counter.name_ref() = counterName;
  *counter.types_ref() = {cfg::CounterType::PACKETS};
  config->trafficCounters_ref()->push_back(counter);
}

/*
 * wedge_agent expects dscp to queue mapping for every possible dscp value.
 * For convenience, this function allows callers to specify only a
 * subset of dscp values, but adds dscp to queue0 (default queue mapping)
 * for missing mappings.
 */
void addMissingDscpMappings(
    std::vector<std::pair<uint16_t, std::vector<int16_t>>>& queue2Dscp) {
  const auto kDefaultQueueId = 0;
  const auto kDscpValueMin = 0;
  const auto kDscpValueMax = 63;
  const auto kDscpValueMaxCnt = kDscpValueMax - kDscpValueMin + 1;

  // get iterator for defaultQueue entry, if missing add and get iterator
  auto defaultQueueIdIter = std::find_if(
      queue2Dscp.begin(),
      queue2Dscp.end(),
      [&](const std::pair<uint16_t, std::vector<int16_t>>& queueAndDscp)
          -> bool { return queueAndDscp.first == kDefaultQueueId; });

  if (defaultQueueIdIter == queue2Dscp.end()) {
    queue2Dscp.push_back({kDefaultQueueId, {}});
    defaultQueueIdIter = std::prev(queue2Dscp.end());
  }

  // Find the dscp values used in queue2Dscp
  std::array<bool, kDscpValueMaxCnt> dscpPresent{false};
  for (auto& pair : queue2Dscp) {
    for (auto& dscpVal : pair.second) {
      if (dscpVal < kDscpValueMin || dscpVal > kDscpValueMax) {
        // the calling test may be testing invalid dscp values
        continue;
      }
      dscpPresent[dscpVal] = true;
    }
  }

  // add dscp to kDefaultQueueId mapping for missing dscp values
  for (auto dscpVal = 0; dscpVal < dscpPresent.size(); dscpVal++) {
    if (!dscpPresent[dscpVal]) {
      defaultQueueIdIter->second.push_back(dscpVal);
    }
  }
}

cfg::QosPolicy* addDscpQosPolicy(
    cfg::SwitchConfig* cfg,
    const std::string& name,
    std::vector<std::pair<uint16_t, std::vector<int16_t>>> map) {
  addMissingDscpMappings(map);

  std::vector<cfg::QosRule> rules;
  for (auto& pair : map) {
    auto qosRule = cfg::QosRule();
    *qosRule.queueId_ref() = pair.first;
    *qosRule.dscp_ref() = pair.second;
    rules.push_back(qosRule);
  }
  auto qosPolicy = cfg::QosPolicy();
  *qosPolicy.name_ref() = name;
  *qosPolicy.rules_ref() = rules;
  cfg->qosPolicies_ref()->push_back(qosPolicy);
  return &cfg->qosPolicies_ref()->back();
}

void delQosPolicy(cfg::SwitchConfig* cfg, const std::string& name) {
  cfg->qosPolicies_ref()->erase(
      std::remove_if(
          cfg->qosPolicies_ref()->begin(),
          cfg->qosPolicies_ref()->end(),
          [&](auto qosPolicy) { return *qosPolicy.name_ref() == name; }),
      cfg->qosPolicies_ref()->end());
}

void setDefaultQosPolicy(cfg::SwitchConfig* cfg, const std::string& name) {
  auto trafficPolicy = cfg->dataPlaneTrafficPolicy_ref();
  if (!trafficPolicy) {
    trafficPolicy = cfg::TrafficPolicyConfig();
  }
  trafficPolicy->defaultQosPolicy_ref() = name;
}

void unsetDefaultQosPolicy(cfg::SwitchConfig* cfg) {
  if (auto trafficPolicy = cfg->dataPlaneTrafficPolicy_ref()) {
    if (auto defaultQosPolicy = trafficPolicy->defaultQosPolicy_ref()) {
      defaultQosPolicy.reset();
    }
  }
}

void overrideQosPolicy(
    cfg::SwitchConfig* cfg,
    int port,
    const std::string& name) {
  auto trafficPolicy = cfg->dataPlaneTrafficPolicy_ref();
  if (!trafficPolicy) {
    trafficPolicy = cfg::TrafficPolicyConfig();
  }
  auto portIdToQosPolicy = trafficPolicy->portIdToQosPolicy_ref();
  if (!portIdToQosPolicy) {
    portIdToQosPolicy = std::map<int, std::string>();
  }
  (*portIdToQosPolicy)[port] = name;
}

void setCPUQosPolicy(cfg::SwitchConfig* cfg, const std::string& name) {
  auto cpuTrafficPolicy = cfg->cpuTrafficPolicy_ref();
  if (!cpuTrafficPolicy) {
    cpuTrafficPolicy = cfg::CPUTrafficPolicyConfig();
  }
  auto trafficPolicy = cpuTrafficPolicy->trafficPolicy_ref();
  if (!trafficPolicy) {
    trafficPolicy = cfg::TrafficPolicyConfig();
  }
  trafficPolicy->defaultQosPolicy_ref() = name;
}

void unsetCPUQosPolicy(cfg::SwitchConfig* cfg) {
  if (auto cpuTrafficPolicy = cfg->cpuTrafficPolicy_ref()) {
    if (auto trafficPolicy = cpuTrafficPolicy->trafficPolicy_ref()) {
      trafficPolicy->defaultQosPolicy_ref().reset();
    }
  }
}

} // namespace facebook::fboss::utility
