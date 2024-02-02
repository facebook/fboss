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
#include <memory>
#include "fboss/agent/hw/switch_asics/HwAsic.h"

#include "fboss/agent/state/SwitchState.h"

DECLARE_bool(enable_acl_table_group);

namespace facebook::fboss::utility {

std::shared_ptr<AclEntry> getAclEntryByName(
    const std::shared_ptr<SwitchState> state,
    const std::string& aclName) {
  std::shared_ptr<AclEntry> swAcl;
  if (FLAGS_enable_acl_table_group) {
    auto aclMap = state->getAclsForTable(
        cfg::AclStage::INGRESS, utility::kDefaultAclTable());
    if (aclMap) {
      swAcl = aclMap->getNodeIf(aclName);
    }
  } else {
    swAcl = state->getAcl(aclName);
  }

  return swAcl;
}

std::optional<cfg::TrafficCounter> getAclTrafficCounter(
    const std::shared_ptr<SwitchState> state,
    const std::string& aclName) {
  auto swAcl = getAclEntryByName(state, aclName);
  if (swAcl && swAcl->getAclAction()) {
    return swAcl->getAclAction()
        ->cref<switch_state_tags::trafficCounter>()
        ->toThrift();
  }
  return std::nullopt;
}

std::string getAclTableGroupName() {
  return "acl-table-group-ingress";
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

cfg::AclEntry* addAclEntry(
    cfg::SwitchConfig* cfg,
    cfg::AclEntry& acl,
    const std::optional<std::string>& tableName) {
  if (FLAGS_enable_acl_table_group) {
    auto aclTableName =
        tableName.has_value() ? tableName.value() : kDefaultAclTable();
    int tableNumber = getAclTableIndex(cfg, aclTableName);
    cfg->aclTableGroup()->aclTables()[tableNumber].aclEntries()->push_back(acl);
    return &cfg->aclTableGroup()->aclTables()[tableNumber].aclEntries()->back();
  } else {
    cfg->acls()->push_back(acl);
    return &cfg->acls()->back();
  }
}

cfg::AclEntry* addAcl(
    cfg::SwitchConfig* cfg,
    const std::string& aclName,
    const cfg::AclActionType& aclActionType,
    const std::optional<std::string>& tableName) {
  auto acl = cfg::AclEntry();
  *acl.name() = aclName;
  *acl.actionType() = aclActionType;

  return addAclEntry(cfg, acl, tableName);
}

std::vector<cfg::AclEntry>& getAcls(
    cfg::SwitchConfig* cfg,
    const std::optional<std::string>& tableName) {
  auto aclTableName =
      tableName.has_value() ? tableName.value() : kDefaultAclTable();
  auto& acls = FLAGS_enable_acl_table_group
      ? *cfg->aclTableGroup()
             ->aclTables()[getAclTableIndex(cfg, aclTableName)]
             .aclEntries()
      : *cfg->acls();

  return acls;
}

void delAcl(
    cfg::SwitchConfig* cfg,
    const std::string& aclName,
    const std::optional<std::string>& tableName) {
  auto& acls = getAcls(cfg, tableName);

  acls.erase(
      std::remove_if(
          acls.begin(),
          acls.end(),
          [&](cfg::AclEntry const& acl) { return *acl.name() == aclName; }),
      acls.end());
}

void delLastAddedAcl(cfg::SwitchConfig* cfg) {
  auto& acls = getAcls(cfg, std::nullopt);
  acls.pop_back();
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

std::string kDefaultAclTable() {
  return "AclTable1";
}

void addDefaultAclTable(cfg::SwitchConfig& cfg) {
  /* Create default ACL table similar to whats being done in Agent today */
  std::vector<cfg::AclTableQualifier> qualifiers = {};
  std::vector<cfg::AclTableActionType> actions = {};
  addAclTable(&cfg, kDefaultAclTable(), 0 /* priority */, actions, qualifiers);
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

std::vector<cfg::CounterType> getAclCounterTypes(const HwSwitch* hwSwitch) {
  // At times, it is non-trivial for SAI implementations to support enabling
  // bytes counters only or packet counters only. In such cases, SAI
  // implementations enable bytes as well as packet counters even if only
  // one of the two is enabled. FBOSS use case does not require enabling
  // only one, but always enables both packets and bytes counters. Thus,
  // enable both in the test. Reference: CS00012271364
  if (hwSwitch->getPlatform()->getAsic()->isSupported(
          HwAsic::Feature::SEPARATE_BYTE_AND_PACKET_ACL_COUNTER)) {
    return {cfg::CounterType::PACKETS};
  } else {
    return {cfg::CounterType::BYTES, cfg::CounterType::PACKETS};
  }
}

} // namespace facebook::fboss::utility
