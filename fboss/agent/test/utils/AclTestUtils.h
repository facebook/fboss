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

#include "fboss/agent/HwSwitch.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/state/AclEntry.h"
#include "fboss/agent/state/SwitchState.h"

class SwSwitch;

namespace facebook::fboss::utility {

using AclStatGetFunc = std::function<int64_t(
    std::shared_ptr<SwitchState>,
    const std::string&,
    const std::string&,
    cfg::AclStage,
    const std::optional<std::string>&)>;

std::string kDefaultAclTable();

cfg::AclEntry* addAclEntry(
    cfg::SwitchConfig* cfg,
    cfg::AclEntry& acl,
    const std::optional<std::string>& tableName);

cfg::AclEntry* addAcl(
    cfg::SwitchConfig* cfg,
    const std::string& aclName,
    const cfg::AclActionType& aclActionType = cfg::AclActionType::PERMIT,
    const std::optional<std::string>& tableName = std::nullopt);

void addEtherTypeToAcl(
    const HwAsic* asic,
    cfg::AclEntry* acl,
    const cfg::EtherType& etherType = cfg::EtherType::IPv6);

std::vector<cfg::AclTableQualifier> genAclQualifiersConfig(
    cfg::AsicType asicType);

int getAclTableIndex(
    cfg::SwitchConfig* cfg,
    const std::optional<std::string>& tableName);

std::shared_ptr<AclEntry> getAclEntryByName(
    const std::shared_ptr<SwitchState> state,
    const std::string& aclName);

std::optional<cfg::TrafficCounter> getAclTrafficCounter(
    const std::shared_ptr<SwitchState> state,
    const std::string& aclName);

std::string getAclTableGroupName();

std::vector<cfg::AclEntry>& getAcls(
    cfg::SwitchConfig* cfg,
    const std::optional<std::string>& tableName);

void delAcl(
    cfg::SwitchConfig* cfg,
    const std::string& aclName,
    const std::optional<std::string>& tableName = std::nullopt);

void delLastAddedAcl(cfg::SwitchConfig* cfg);

void addAclTableGroup(
    cfg::SwitchConfig* cfg,
    cfg::AclStage aclStage,
    const std::string& aclTableGroupName = "AclTableGroup1");

void addDefaultAclTable(cfg::SwitchConfig& cfg);

cfg::AclTable* addAclTable(
    cfg::SwitchConfig* cfg,
    const std::string& aclTableName,
    const int aclTablePriority,
    const std::vector<cfg::AclTableActionType>& actionTypes,
    const std::vector<cfg::AclTableQualifier>& qualifiers);

void delAclTable(cfg::SwitchConfig* cfg, const std::string& aclTableName);

void addAclStat(
    cfg::SwitchConfig* cfg,
    const std::string& matcher,
    const std::string& counterName,
    std::vector<cfg::CounterType> counterTypes = {});

void delAclStat(
    cfg::SwitchConfig* cfg,
    const std::string& matcher,
    const std::string& counterName);

void addAclMatchActions(
    cfg::SwitchConfig* cfg,
    const std::string& matcher,
    const std::optional<std::string>& counterName,
    const std::optional<std::string>& mirrorName,
    bool ingress = true);

void renameAclStat(
    cfg::SwitchConfig* cfg,
    const std::string& matcher,
    const std::string& oldCounterName,
    const std::string& newCounterName);

std::vector<cfg::CounterType> getAclCounterTypes(
    const std::vector<const HwAsic*>& asics);

uint64_t getAclInOutPackets(
    const SwSwitch* sw,
    const std::string& statName,
    bool bytes = false);

uint64_t getAclInOutPackets(
    const HwSwitch* hw,
    const std::string& statName,
    bool bytes = false);

template <typename SwitchT>
AclStatGetFunc getAclStatGetFn(SwitchT* sw) {
  return [sw](
             std::shared_ptr<SwitchState> /*state*/,
             const std::string& /*aclName*/,
             const std::string& statName,
             cfg::AclStage /*aclStage*/,
             const std::optional<std::string>& /*aclTableName*/) {
    return getAclInOutPackets(sw, statName);
  };
}

std::shared_ptr<AclEntry> getAclEntry(
    const std::shared_ptr<SwitchState>& state,
    const std::string& name,
    bool enableAclTableGroup);

} // namespace facebook::fboss::utility
