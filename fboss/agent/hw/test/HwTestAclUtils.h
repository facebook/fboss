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
int getAclTableNumAclEntries(const HwSwitch* hwSwitch);

void checkSwHwAclMatch(
    const HwSwitch* hw,
    std::shared_ptr<SwitchState> state,
    const std::string& aclName);

bool isAclTableEnabled(const HwSwitch* hwSwitch);

template <typename T>
bool isQualifierPresent(
    const HwSwitch* hwSwitch,
    const std::shared_ptr<SwitchState>& state,
    const std::string& aclName);

void checkAclEntryAndStatCount(
    const HwSwitch* hwSwitch,
    int aclCount,
    int aclStatCount,
    int counterCount);

std::optional<cfg::TrafficCounter> getAclTrafficCounter(
    const std::shared_ptr<SwitchState> state,
    const std::string& aclName);

void checkAclStat(
    const HwSwitch* hwSwitch,
    std::shared_ptr<SwitchState> state,
    std::vector<std::string> acls,
    const std::string& statName,
    std::vector<cfg::CounterType> counterTypes = {cfg::CounterType::PACKETS});

void checkAclStatDeleted(const HwSwitch* hwSwitch, const std::string& statName);

void checkAclStatSize(const HwSwitch* hwSwitch, const std::string& statName);

cfg::AclEntry* addAcl(
    cfg::SwitchConfig* cfg,
    const std::string& aclName,
    const cfg::AclActionType& aclActionType = cfg::AclActionType::PERMIT);

void delAcl(cfg::SwitchConfig* cfg, const std::string& aclName);

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
    const std::string& statName);

} // namespace facebook::fboss::utility
