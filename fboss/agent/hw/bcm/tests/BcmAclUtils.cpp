/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/bcm/tests/BcmAclUtils.h"

namespace facebook::fboss::utility {

cfg::AclEntry* addAcl(cfg::SwitchConfig* cfg, const std::string& aclName) {
  auto acl = cfg::AclEntry();
  acl.name = aclName;
  acl.actionType = cfg::AclActionType::PERMIT;
  cfg->acls.push_back(acl);
  return &cfg->acls.back();
}

void delAcl(cfg::SwitchConfig* cfg, const std::string& aclName) {
  auto& acls = cfg->acls;
  acls.erase(
      std::remove_if(
          acls.begin(),
          acls.end(),
          [&](cfg::AclEntry const& acl) { return acl.name == aclName; }),
      acls.end());
}

void addAclStat(
    cfg::SwitchConfig* cfg,
    const std::string& matcher,
    const std::string& counterName,
    std::vector<cfg::CounterType> counterTypes) {
  auto counter = cfg::TrafficCounter();
  counter.name = counterName;
  if (!counterTypes.empty()) {
    counter.types = counterTypes;
  }
  bool counterExists = false;
  for (auto& c : cfg->trafficCounters) {
    if (c.name == counterName) {
      counterExists = true;
      break;
    }
  }
  if (!counterExists) {
    cfg->trafficCounters.push_back(counter);
  }

  auto matchAction = cfg::MatchAction();
  matchAction.counter_ref() = counterName;
  auto action = cfg::MatchToAction();
  action.matcher = matcher;
  action.action = matchAction;

  if (!cfg->dataPlaneTrafficPolicy_ref()) {
    cfg->dataPlaneTrafficPolicy_ref() = cfg::TrafficPolicyConfig();
  }
  cfg->dataPlaneTrafficPolicy_ref()->matchToAction.push_back(action);
}

void delAclStat(
    cfg::SwitchConfig* cfg,
    const std::string& matcher,
    const std::string& counterName) {
  int refCnt = 0;
  if (auto dataPlaneTrafficPolicy = cfg->dataPlaneTrafficPolicy_ref()) {
    auto& matchActions = dataPlaneTrafficPolicy->matchToAction;
    matchActions.erase(
        std::remove_if(
            matchActions.begin(),
            matchActions.end(),
            [&](cfg::MatchToAction const& matchAction) {
              if (matchAction.matcher == matcher &&
                  matchAction.action.counter_ref().value_or({}) ==
                      counterName) {
                ++refCnt;
                return true;
              }
              return false;
            }),
        matchActions.end());
  }
  if (refCnt == 0) {
    auto& counters = cfg->trafficCounters;
    counters.erase(
        std::remove_if(
            counters.begin(),
            counters.end(),
            [&](cfg::TrafficCounter const& counter) {
              return counter.name == counterName;
            }),
        counters.end());
  }
}

void renameAclStat(
    cfg::SwitchConfig* cfg,
    const std::string& matcher,
    const std::string& oldCounterName,
    const std::string& newCounterName) {
  delAclStat(cfg, matcher, oldCounterName);
  addAclStat(cfg, matcher, newCounterName);
}

} // namespace facebook::fboss::utility
