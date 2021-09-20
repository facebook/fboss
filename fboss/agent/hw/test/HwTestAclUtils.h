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

namespace facebook::fboss::utility {
// TODO (skhare)
// When SwitchState is extended to carry AclTable, pass AclTable handle to
// getAclTableNumAclEntries.
int getAclTableNumAclEntries(
    const HwSwitch* hwSwitch,
    const std::optional<std::string>& aclTableName = std::nullopt);

void checkSwHwAclMatch(
    const HwSwitch* hw,
    std::shared_ptr<SwitchState> state,
    const std::string& aclName,
    cfg::AclStage aclStage = cfg::AclStage::INGRESS,
    const std::optional<std::string>& aclTableName = std::nullopt);

bool isAclTableEnabled(
    const HwSwitch* hwSwitch,
    const std::optional<std::string>& aclTableName = std::nullopt);

template <typename T>
bool isQualifierPresent(
    const HwSwitch* hwSwitch,
    const std::shared_ptr<SwitchState>& state,
    const std::string& aclName);

void checkAclEntryAndStatCount(
    const HwSwitch* hwSwitch,
    int aclCount,
    int aclStatCount,
    int counterCount,
    const std::optional<std::string>& aclTableName = std::nullopt);

std::optional<cfg::TrafficCounter> getAclTrafficCounter(
    const std::shared_ptr<SwitchState> state,
    const std::string& aclName);

void checkAclStat(
    const HwSwitch* hwSwitch,
    std::shared_ptr<SwitchState> state,
    std::vector<std::string> acls,
    const std::string& statName,
    std::vector<cfg::CounterType> counterTypes = {cfg::CounterType::PACKETS},
    const std::optional<std::string>& aclTableName = std::nullopt);

void checkAclStatDeleted(const HwSwitch* hwSwitch, const std::string& statName);

void checkAclStatSize(const HwSwitch* hwSwitch, const std::string& statName);

int getAclTableIndex(
    cfg::SwitchConfig* cfg,
    const std::optional<std::string>& tableName);

cfg::AclEntry* addAcl(
    cfg::SwitchConfig* cfg,
    const std::string& aclName,
    const cfg::AclActionType& aclActionType = cfg::AclActionType::PERMIT,
    const std::optional<std::string>& tableName = std::nullopt);

void delAcl(
    cfg::SwitchConfig* cfg,
    const std::string& aclName,
    const std::optional<std::string>& tableName = std::nullopt);

void addAclTableGroup(
    cfg::SwitchConfig* cfg,
    cfg::AclStage aclStage,
    const std::string& aclTableGroupName = "AclTableGroup1");

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

void renameAclStat(
    cfg::SwitchConfig* cfg,
    const std::string& matcher,
    const std::string& oldCounterName,
    const std::string& newCounterName);

uint64_t getAclInOutPackets(
    const HwSwitch* hwSwitch,
    std::shared_ptr<SwitchState> state,
    const std::string& aclName,
    const std::string& statName,
    cfg::AclStage aclStage = cfg::AclStage::INGRESS,
    const std::optional<std::string>& aclTableName = std::nullopt);

} // namespace facebook::fboss::utility
