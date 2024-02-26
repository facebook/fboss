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
#include "fboss/agent/test/utils/AclTestUtils.h"

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

bool verifyAclEnabled(const HwSwitch* hwSwitch);

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

void checkAclStat(
    const HwSwitch* hwSwitch,
    std::shared_ptr<SwitchState> state,
    std::vector<std::string> acls,
    const std::string& statName,
    std::vector<cfg::CounterType> counterTypes,
    const std::optional<std::string>& aclTableName = std::nullopt);

void checkAclStatDeleted(const HwSwitch* hwSwitch, const std::string& statName);

void checkAclStatSize(const HwSwitch* hwSwitch, const std::string& statName);

uint64_t getAclInOutPackets(
    const HwSwitch* hwSwitch,
    std::shared_ptr<SwitchState> state,
    const std::string& aclName,
    const std::string& statName,
    cfg::AclStage aclStage = cfg::AclStage::INGRESS,
    const std::optional<std::string>& aclTableName = std::nullopt);

uint64_t getAclInOutBytes(
    const HwSwitch* hwSwitch,
    std::shared_ptr<SwitchState> state,
    const std::string& aclName,
    const std::string& statName,
    cfg::AclStage aclStage = cfg::AclStage::INGRESS,
    const std::optional<std::string>& aclTableName = std::nullopt);

void checkSwAclSendToQueue(
    std::shared_ptr<SwitchState> state,
    const std::string& aclName,
    bool sendToCPU,
    int queueId);

} // namespace facebook::fboss::utility
