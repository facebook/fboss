/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/test/HwTestAclUtils.h"

#include "fboss/agent/state/SwitchState.h"

namespace facebook::fboss::utility {

std::optional<cfg::TrafficCounter> getAclTrafficCounter(
    const std::shared_ptr<SwitchState> state,
    const std::string& aclName) {
  auto swAcl = state->getAcl(aclName);
  if (swAcl && swAcl->getAclAction()) {
    return swAcl->getAclAction()->getTrafficCounter();
  }
  return std::nullopt;
}

cfg::AclEntry* addAcl(
    cfg::SwitchConfig* cfg,
    const std::string& aclName,
    const cfg::AclActionType& aclActionType) {
  auto acl = cfg::AclEntry();
  *acl.name_ref() = aclName;
  *acl.actionType_ref() = aclActionType;
  cfg->acls_ref()->push_back(acl);
  return &cfg->acls_ref()->back();
}

void delAcl(cfg::SwitchConfig* cfg, const std::string& aclName) {
  auto& acls = *cfg->acls_ref();
  acls.erase(
      std::remove_if(
          acls.begin(),
          acls.end(),
          [&](cfg::AclEntry const& acl) { return *acl.name_ref() == aclName; }),
      acls.end());
}

void addAclStat(
    cfg::SwitchConfig* cfg,
    const std::string& matcher,
    const std::string& counterName,
    std::vector<cfg::CounterType> counterTypes) {
  auto counter = cfg::TrafficCounter();
  *counter.name_ref() = counterName;
  if (!counterTypes.empty()) {
    *counter.types_ref() = counterTypes;
  }
  bool counterExists = false;
  for (auto& c : *cfg->trafficCounters_ref()) {
    if (*c.name_ref() == counterName) {
      counterExists = true;
      break;
    }
  }
  if (!counterExists) {
    cfg->trafficCounters_ref()->push_back(counter);
  }

  auto matchAction = cfg::MatchAction();
  matchAction.counter_ref() = counterName;
  auto action = cfg::MatchToAction();
  *action.matcher_ref() = matcher;
  *action.action_ref() = matchAction;

  if (!cfg->dataPlaneTrafficPolicy_ref()) {
    cfg->dataPlaneTrafficPolicy_ref() = cfg::TrafficPolicyConfig();
  }
  cfg->dataPlaneTrafficPolicy_ref()->matchToAction_ref()->push_back(action);
}

void delAclStat(
    cfg::SwitchConfig* cfg,
    const std::string& matcher,
    const std::string& counterName) {
  int refCnt = 0;
  if (auto dataPlaneTrafficPolicy = cfg->dataPlaneTrafficPolicy_ref()) {
    auto& matchActions = *dataPlaneTrafficPolicy->matchToAction_ref();
    matchActions.erase(
        std::remove_if(
            matchActions.begin(),
            matchActions.end(),
            [&](cfg::MatchToAction const& matchAction) {
              if (*matchAction.matcher_ref() == matcher &&
                  matchAction.action_ref()->counter_ref().value_or({}) ==
                      counterName) {
                ++refCnt;
                return true;
              }
              return false;
            }),
        matchActions.end());
  }
  if (refCnt == 0) {
    auto& counters = *cfg->trafficCounters_ref();
    counters.erase(
        std::remove_if(
            counters.begin(),
            counters.end(),
            [&](cfg::TrafficCounter const& counter) {
              return *counter.name_ref() == counterName;
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
