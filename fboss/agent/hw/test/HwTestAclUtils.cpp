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
    return swAcl->getAclAction()
        ->cref<switch_state_tags::trafficCounter>()
        ->toThrift();
  }
  return std::nullopt;
}

int getAclTableIndex(
    cfg::SwitchConfig* cfg,
    const std::optional<std::string>& tableName) {
  if (!cfg->aclTableGroup()) {
    throw FbossError(
        "Multiple acl tables flag enabled but config leaves aclTableGroup empty");
  }
  if (!tableName.has_value()) {
    throw FbossError(
        "Multiple acl tables flag enabled but no acl table name provided for add/delAcl()");
  }
  int tableIndex;
  std::vector<cfg::AclTable> aclTables = *cfg->aclTableGroup()->aclTables();
  std::vector<cfg::AclTable>::iterator it = std::find_if(
      aclTables.begin(), aclTables.end(), [&](cfg::AclTable const& aclTable) {
        return *aclTable.name() == tableName.value();
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
  *acl.name() = aclName;
  *acl.actionType() = aclActionType;

  if (FLAGS_enable_acl_table_group) {
    int tableNumber = getAclTableIndex(cfg, tableName);
    cfg->aclTableGroup()->aclTables()[tableNumber].aclEntries()->push_back(acl);
    return &cfg->aclTableGroup()->aclTables()[tableNumber].aclEntries()->back();
  } else {
    cfg->acls()->push_back(acl);
    return &cfg->acls()->back();
  }
}

void delAcl(
    cfg::SwitchConfig* cfg,
    const std::string& aclName,
    const std::optional<std::string>& tableName) {
  auto& acls = FLAGS_enable_acl_table_group
      ? *cfg->aclTableGroup()
             ->aclTables()[getAclTableIndex(cfg, tableName)]
             .aclEntries()
      : *cfg->acls();

  acls.erase(
      std::remove_if(
          acls.begin(),
          acls.end(),
          [&](cfg::AclEntry const& acl) { return *acl.name() == aclName; }),
      acls.end());
}

void addAclTableGroup(
    cfg::SwitchConfig* cfg,
    cfg::AclStage aclStage,
    const std::string& aclTableGroupName) {
  cfg::AclTableGroup cfgTableGroup;
  cfg->aclTableGroup() = cfgTableGroup;
  cfg->aclTableGroup()->name() = aclTableGroupName;
  cfg->aclTableGroup()->stage() = aclStage;
}

cfg::AclTable* addAclTable(
    cfg::SwitchConfig* cfg,
    const std::string& aclTableName,
    const int aclTablePriority,
    const std::vector<cfg::AclTableActionType>& actionTypes,
    const std::vector<cfg::AclTableQualifier>& qualifiers) {
  if (!cfg->aclTableGroup()) {
    throw FbossError(
        "Attempted to add acl table without first creating acl table group");
  }

  cfg::AclTable aclTable;
  aclTable.name() = aclTableName;
  aclTable.priority() = aclTablePriority;
  aclTable.actionTypes() = actionTypes;
  aclTable.qualifiers() = qualifiers;

  cfg->aclTableGroup()->aclTables()->push_back(aclTable);
  return &cfg->aclTableGroup()->aclTables()->back();
}

void delAclTable(cfg::SwitchConfig* cfg, const std::string& aclTableName) {
  if (!cfg->aclTableGroup()) {
    throw FbossError(
        "Attempted to delete acl table (",
        aclTableName,
        ") from uninstantiated table group");
  }
  auto& aclTables = *cfg->aclTableGroup()->aclTables();
  aclTables.erase(
      std::remove_if(
          aclTables.begin(),
          aclTables.end(),
          [&](cfg::AclTable const& aclTable) {
            return *aclTable.name() == aclTableName;
          }),
      aclTables.end());
}

void addAclStat(
    cfg::SwitchConfig* cfg,
    const std::string& matcher,
    const std::string& counterName,
    std::vector<cfg::CounterType> counterTypes) {
  auto counter = cfg::TrafficCounter();
  *counter.name() = counterName;
  if (!counterTypes.empty()) {
    *counter.types() = counterTypes;
  }
  bool counterExists = false;
  for (auto& c : *cfg->trafficCounters()) {
    if (*c.name() == counterName) {
      counterExists = true;
      break;
    }
  }
  if (!counterExists) {
    cfg->trafficCounters()->push_back(counter);
  }

  auto matchAction = cfg::MatchAction();
  matchAction.counter() = counterName;
  auto action = cfg::MatchToAction();
  *action.matcher() = matcher;
  *action.action() = matchAction;

  if (!cfg->dataPlaneTrafficPolicy()) {
    cfg->dataPlaneTrafficPolicy() = cfg::TrafficPolicyConfig();
  }
  cfg->dataPlaneTrafficPolicy()->matchToAction()->push_back(action);
}

void delAclStat(
    cfg::SwitchConfig* cfg,
    const std::string& matcher,
    const std::string& counterName) {
  int refCnt = 0;
  if (auto dataPlaneTrafficPolicy = cfg->dataPlaneTrafficPolicy()) {
    auto& matchActions = *dataPlaneTrafficPolicy->matchToAction();
    matchActions.erase(
        std::remove_if(
            matchActions.begin(),
            matchActions.end(),
            [&](cfg::MatchToAction const& matchAction) {
              if (*matchAction.matcher() == matcher &&
                  matchAction.action()->counter().value_or({}) == counterName) {
                ++refCnt;
                return true;
              }
              return false;
            }),
        matchActions.end());
  }
  if (refCnt == 0) {
    auto& counters = *cfg->trafficCounters();
    counters.erase(
        std::remove_if(
            counters.begin(),
            counters.end(),
            [&](cfg::TrafficCounter const& counter) {
              return *counter.name() == counterName;
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
