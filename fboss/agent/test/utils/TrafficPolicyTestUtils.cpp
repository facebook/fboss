/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/test/utils/TrafficPolicyTestUtils.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/test/utils/AclTestUtils.h"
#include "fboss/agent/test/utils/CoppTestUtils.h"

DECLARE_bool(enable_acl_table_group);

namespace facebook::fboss::utility {
cfg::AclEntry* addDscpAclToCfg(
    const HwAsic* hwAsic,
    cfg::SwitchConfig* config,
    const std::string& aclName,
    uint32_t dscp) {
  auto acl = cfg::AclEntry();
  *acl.name() = aclName;
  acl.dscp() = dscp;
  utility::addEtherTypeToAcl(hwAsic, &acl, cfg::EtherType::IPv6);

  return utility::addAclEntry(config, acl, utility::kDefaultAclTable());
}

void addL4SrcPortAclToCfg(
    const HwAsic* hwAsic,
    cfg::SwitchConfig* config,
    const std::string& aclName,
    IP_PROTO proto,
    uint32_t l4SrcPort) {
  auto acl = cfg::AclEntry();
  *acl.name() = aclName;
  acl.proto() = static_cast<int>(proto);
  acl.l4SrcPort() = l4SrcPort;
  utility::addEtherTypeToAcl(hwAsic, &acl, cfg::EtherType::IPv6);

  utility::addAclEntry(config, acl, utility::kDefaultAclTable());
}

void addL4DstPortAclToCfg(
    const HwAsic* hwAsic,
    cfg::SwitchConfig* config,
    const std::string& aclName,
    IP_PROTO proto,
    uint32_t l4DstPort) {
  auto acl = cfg::AclEntry();
  *acl.name() = aclName;
  acl.proto() = static_cast<int>(proto);
  acl.l4DstPort() = l4DstPort;
  utility::addEtherTypeToAcl(hwAsic, &acl, cfg::EtherType::IPv6);

  utility::addAclEntry(config, acl, utility::kDefaultAclTable());
}

void addSetDscpAndEgressQueueActionToCfg(
    cfg::SwitchConfig* config,
    const std::string& aclName,
    uint8_t dscp,
    int queueId,
    bool isSai) {
  cfg::MatchAction matchAction = utility::getToQueueAction(queueId, isSai);

  // set specific dscp value action
  cfg::SetDscpMatchAction setDscpMatchAction;
  setDscpMatchAction.dscpValue() = dscp;
  matchAction.setDscp() = setDscpMatchAction;

  utility::addMatcher(config, aclName, matchAction);
}

void addL2ClassIDAndTtlAcl(
    cfg::SwitchConfig* config,
    const std::string& aclName,
    cfg::AclLookupClass lookupClassL2,
    std::optional<cfg::Ttl> ttl) {
  auto acl = cfg::AclEntry();
  *acl.name() = aclName;
  acl.lookupClassL2() = lookupClassL2;
  if (ttl.has_value()) {
    acl.ttl() = ttl.value();
  }

  utility::addAclEntry(config, acl, utility::kDefaultAclTable());
}

void addNeighborClassIDAndTtlAcl(
    cfg::SwitchConfig* config,
    const std::string& aclName,
    cfg::AclLookupClass lookupClassNeighbor,
    std::optional<cfg::Ttl> ttl) {
  auto acl = cfg::AclEntry();
  *acl.name() = aclName;
  acl.lookupClassNeighbor() = lookupClassNeighbor;
  if (ttl.has_value()) {
    acl.ttl() = ttl.value();
  }

  utility::addAclEntry(config, acl, utility::kDefaultAclTable());
}

void addL2ClassIDDropAcl(
    cfg::SwitchConfig* config,
    const std::string& aclName,
    cfg::AclLookupClass lookupClassL2) {
  auto acl = cfg::AclEntry();
  *acl.name() = aclName;
  acl.lookupClassL2() = lookupClassL2;
  acl.actionType() = cfg::AclActionType::DENY;

  utility::addAclEntry(config, acl, utility::kDefaultAclTable());
}

void addNeighborClassIDDropAcl(
    cfg::SwitchConfig* config,
    const std::string& aclName,
    cfg::AclLookupClass lookupClassNeighbor) {
  auto acl = cfg::AclEntry();
  *acl.name() = aclName;
  acl.lookupClassNeighbor() = lookupClassNeighbor;
  acl.actionType() = cfg::AclActionType::DENY;

  utility::addAclEntry(config, acl, utility::kDefaultAclTable());
}

void addRouteClassIDDropAcl(
    cfg::SwitchConfig* config,
    const std::string& aclName,
    cfg::AclLookupClass lookupClassRoute) {
  auto acl = cfg::AclEntry();
  *acl.name() = aclName;
  acl.lookupClassRoute() = lookupClassRoute;
  acl.actionType() = cfg::AclActionType::DENY;

  utility::addAclEntry(config, acl, utility::kDefaultAclTable());
}

void addRouteClassIDAndTtlAcl(
    cfg::SwitchConfig* config,
    const std::string& aclName,
    cfg::AclLookupClass lookupClassRoute,
    std::optional<cfg::Ttl> ttl) {
  auto acl = cfg::AclEntry();
  *acl.name() = aclName;
  acl.lookupClassRoute() = lookupClassRoute;
  if (ttl.has_value()) {
    acl.ttl() = ttl.value();
  }

  utility::addAclEntry(config, acl, utility::kDefaultAclTable());
}

void addQueueMatcher(
    cfg::SwitchConfig* config,
    const std::string& matcherName,
    int queueId,
    bool isSai,
    const std::optional<std::string>& counterName) {
  cfg::MatchAction matchAction = utility::getToQueueAction(queueId, isSai);

  if (counterName.has_value()) {
    matchAction.counter() = counterName.value();
  }

  utility::addMatcher(config, matcherName, matchAction);
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
    *qosRule.queueId() = pair.first;
    *qosRule.dscp() = pair.second;
    rules.push_back(qosRule);
  }
  cfg::QosMap qosMap;
  for (int q = 0; q < 8; q++) {
    qosMap.trafficClassToQueueId()->emplace(q, q);
  }
  auto qosPolicy = cfg::QosPolicy();
  *qosPolicy.name() = name;
  *qosPolicy.rules() = rules;
  qosPolicy.qosMap() = qosMap;
  cfg->qosPolicies()->push_back(qosPolicy);
  return &cfg->qosPolicies()->back();
}

void delQosPolicy(cfg::SwitchConfig* cfg, const std::string& name) {
  cfg->qosPolicies()->erase(
      std::remove_if(
          cfg->qosPolicies()->begin(),
          cfg->qosPolicies()->end(),
          [&](auto qosPolicy) { return *qosPolicy.name() == name; }),
      cfg->qosPolicies()->end());
}

void setDefaultQosPolicy(cfg::SwitchConfig* cfg, const std::string& name) {
  auto trafficPolicy = cfg->dataPlaneTrafficPolicy();
  if (!trafficPolicy) {
    trafficPolicy = cfg::TrafficPolicyConfig();
  }
  trafficPolicy->defaultQosPolicy() = name;
}

void unsetDefaultQosPolicy(cfg::SwitchConfig* cfg) {
  if (auto trafficPolicy = cfg->dataPlaneTrafficPolicy()) {
    if (auto defaultQosPolicy = trafficPolicy->defaultQosPolicy()) {
      defaultQosPolicy.reset();
    }
  }
}

void overrideQosPolicy(
    cfg::SwitchConfig* cfg,
    int port,
    const std::string& name) {
  auto trafficPolicy = cfg->dataPlaneTrafficPolicy();
  if (!trafficPolicy) {
    trafficPolicy = cfg::TrafficPolicyConfig();
  }
  auto portIdToQosPolicy = trafficPolicy->portIdToQosPolicy();
  if (!portIdToQosPolicy) {
    portIdToQosPolicy = std::map<int, std::string>();
  }
  (*portIdToQosPolicy)[port] = name;
}

void setCPUQosPolicy(cfg::SwitchConfig* cfg, const std::string& name) {
  auto cpuTrafficPolicy = cfg->cpuTrafficPolicy();
  if (!cpuTrafficPolicy) {
    cpuTrafficPolicy = cfg::CPUTrafficPolicyConfig();
  }
  auto trafficPolicy = cpuTrafficPolicy->trafficPolicy();
  if (!trafficPolicy) {
    trafficPolicy = cfg::TrafficPolicyConfig();
  }
  trafficPolicy->defaultQosPolicy() = name;
}

void unsetCPUQosPolicy(cfg::SwitchConfig* cfg) {
  if (auto cpuTrafficPolicy = cfg->cpuTrafficPolicy()) {
    if (auto trafficPolicy = cpuTrafficPolicy->trafficPolicy()) {
      trafficPolicy->defaultQosPolicy().reset();
    }
  }
}

} // namespace facebook::fboss::utility
