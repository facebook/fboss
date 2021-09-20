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

DECLARE_bool(enable_acl_table_group);

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

int getAclTableIndex(
    cfg::SwitchConfig* cfg,
    const std::optional<std::string>& tableName) {
  if (!cfg->aclTableGroup_ref()) {
    throw FbossError(
        "Multiple acl tables flag enabled but config leaves aclTableGroup empty");
  }
  if (!tableName.has_value()) {
    throw FbossError(
        "Multiple acl tables flag enabled but no acl table name provided for add/delAcl()");
  }
  int tableIndex;
  std::vector<cfg::AclTable> aclTables =
      *cfg->aclTableGroup_ref()->aclTables_ref();
  std::vector<cfg::AclTable>::iterator it = std::find_if(
      aclTables.begin(), aclTables.end(), [&](cfg::AclTable const& aclTable) {
        return *aclTable.name_ref() == tableName.value();
      });
  if (it != aclTables.end()) {
    tableIndex = std::distance(aclTables.begin(), it);
  } else {
    throw FbossError(
        "Table with name ", tableName.value(), " does not exist in config");
  }
  return tableIndex;
}

cfg::AclEntry* addAcl(
    cfg::SwitchConfig* cfg,
    const std::string& aclName,
    const cfg::AclActionType& aclActionType,
    const std::optional<std::string>& tableName) {
  auto acl = cfg::AclEntry();
  *acl.name_ref() = aclName;
  *acl.actionType_ref() = aclActionType;

  if (FLAGS_enable_acl_table_group) {
    int tableNumber = getAclTableIndex(cfg, tableName);
    cfg->aclTableGroup_ref()
        ->aclTables_ref()[tableNumber]
        .aclEntries_ref()
        ->push_back(acl);
    return &cfg->aclTableGroup_ref()
                ->aclTables_ref()[tableNumber]
                .aclEntries_ref()
                ->back();
  } else {
    cfg->acls_ref()->push_back(acl);
    return &cfg->acls_ref()->back();
  }
}

void delAcl(
    cfg::SwitchConfig* cfg,
    const std::string& aclName,
    const std::optional<std::string>& tableName) {
  auto& acls = FLAGS_enable_acl_table_group
      ? *cfg->aclTableGroup_ref()
             ->aclTables_ref()[getAclTableIndex(cfg, tableName)]
             .aclEntries_ref()
      : *cfg->acls_ref();

  acls.erase(
      std::remove_if(
          acls.begin(),
          acls.end(),
          [&](cfg::AclEntry const& acl) { return *acl.name_ref() == aclName; }),
      acls.end());
}

void addAclTableGroup(
    cfg::SwitchConfig* cfg,
    cfg::AclStage aclStage,
    const std::string& aclTableGroupName) {
  cfg::AclTableGroup cfgTableGroup;
  cfg->aclTableGroup_ref() = cfgTableGroup;
  cfg->aclTableGroup_ref()->name_ref() = aclTableGroupName;
  cfg->aclTableGroup_ref()->stage_ref() = aclStage;
}

cfg::AclTable* addAclTable(
    cfg::SwitchConfig* cfg,
    const std::string& aclTableName,
    const int aclTablePriority,
    const std::vector<cfg::AclTableActionType>& actionTypes,
    const std::vector<cfg::AclTableQualifier>& qualifiers) {
  if (!cfg->aclTableGroup_ref()) {
    throw FbossError(
        "Attempted to add acl table without first creating acl table group");
  }

  cfg::AclTable aclTable;
  aclTable.name_ref() = aclTableName;
  aclTable.priority_ref() = aclTablePriority;
  aclTable.actionTypes_ref() = actionTypes;
  aclTable.qualifiers_ref() = qualifiers;

  cfg->aclTableGroup_ref()->aclTables_ref()->push_back(aclTable);
  return &cfg->aclTableGroup_ref()->aclTables_ref()->back();
}

void delAclTable(cfg::SwitchConfig* cfg, const std::string& aclTableName) {
  if (!cfg->aclTableGroup_ref()) {
    throw FbossError(
        "Attempted to delete acl table (",
        aclTableName,
        ") from uninstantiated table group");
  }
  auto& aclTables = *cfg->aclTableGroup_ref()->aclTables_ref();
  aclTables.erase(
      std::remove_if(
          aclTables.begin(),
          aclTables.end(),
          [&](cfg::AclTable const& aclTable) {
            return *aclTable.name_ref() == aclTableName;
          }),
      aclTables.end());
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
