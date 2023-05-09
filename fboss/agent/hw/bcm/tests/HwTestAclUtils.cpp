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

#include "fboss/agent/hw/bcm/BcmAclEntry.h"
#include "fboss/agent/hw/bcm/BcmAclStat.h"
#include "fboss/agent/hw/bcm/BcmAclTable.h"
#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmFieldProcessorUtils.h"
#include "fboss/agent/hw/bcm/BcmIngressFieldProcessorFlexCounter.h"
#include "fboss/agent/hw/bcm/BcmStatUpdater.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/state/SwitchState.h"

#include <gtest/gtest.h>

extern "C" {
#include <bcm/field.h>
}

namespace {
static const std::vector<facebook::fboss::cfg::CounterType> kPacketCounters = {
    facebook::fboss::cfg::CounterType::PACKETS};
static const std::vector<facebook::fboss::cfg::CounterType> kByteCounters = {
    facebook::fboss::cfg::CounterType::BYTES};
} // namespace

namespace facebook::fboss::utility {
int getAclTableNumAclEntries(
    const HwSwitch* hwSwitch,
    const std::optional<std::string>& /*aclTableName*/) {
  auto bcmSwitch = static_cast<const BcmSwitch*>(hwSwitch);

  bcm_field_group_t gid =
      bcmSwitch->getPlatform()->getAsic()->getDefaultACLGroupID();
  int size;

  auto rv =
      bcm_field_entry_multi_get(bcmSwitch->getUnit(), gid, 0, nullptr, &size);
  bcmCheckError(
      rv,
      "failed to get field group entry count, gid=",
      folly::to<std::string>(gid));
  return size;
}

void checkSwHwAclMatch(
    const HwSwitch* hwSwitch,
    std::shared_ptr<SwitchState> state,
    const std::string& aclName,
    cfg::AclStage /* aclStage */,
    const std::optional<std::string>& /*aclTableName*/) {
  auto bcmSwitch = static_cast<const BcmSwitch*>(hwSwitch);

  auto swAcl = state->getMultiSwitchAcls()->getNodeIf(aclName);
  ASSERT_NE(nullptr, swAcl);
  auto hwAcl = bcmSwitch->getAclTable()->getAclIf(swAcl->getPriority());
  ASSERT_NE(nullptr, hwAcl);
  ASSERT_TRUE(BcmAclEntry::isStateSame(
      bcmSwitch,
      bcmSwitch->getPlatform()->getAsic()->getDefaultACLGroupID(),
      hwAcl->getHandle(),
      swAcl));
}

bool isAclTableEnabled(
    const HwSwitch* hwSwitch,
    const std::optional<std::string>& /*aclTableName*/) {
  auto bcmSwitch = static_cast<const BcmSwitch*>(hwSwitch);

  bcm_field_group_t gid =
      bcmSwitch->getPlatform()->getAsic()->getDefaultACLGroupID();
  int enable = -1;

  auto rv = bcm_field_group_enable_get(bcmSwitch->getUnit(), gid, &enable);
  bcmCheckError(rv, "failed to get field group enable status");
  CHECK(enable == 0 || enable == 1);
  return (enable == 1);
}

bool verifyAclEnabled(const HwSwitch* hwSwitch) {
  return isAclTableEnabled(hwSwitch);
}

template bool isQualifierPresent<cfg::IpFragMatch>(
    const HwSwitch* hwSwitch,
    const std::shared_ptr<SwitchState>& state,
    const std::string& aclName);

template <typename T>
bool isQualifierPresent(
    const HwSwitch* hwSwitch,
    const std::shared_ptr<SwitchState>& state,
    const std::string& aclName) {
  auto bcmSwitch = static_cast<const BcmSwitch*>(hwSwitch);

  auto swAcl = state->getMultiSwitchAcls()->getNodeIf(aclName);
  auto hwAcl = bcmSwitch->getAclTable()->getAclIf(swAcl->getPriority());

  bcm_field_IpFrag_t hwValueIpFrag{};
  auto ret = bcm_field_qualify_IpFrag_get(
      bcmSwitch->getUnit(), hwAcl->getHandle(), &hwValueIpFrag);

  // In broadcom side, when it's asked to get a non-existing qualifier for an
  // enum, it will return BCM_E_INTERNAL or BCM_E_NOT_FOUND
  return !(ret == BCM_E_INTERNAL || ret == BCM_E_NOT_FOUND);
}

void checkAclEntryAndStatCount(
    const HwSwitch* hwSwitch,
    int aclCount,
    int aclStatCount,
    int counterCount,
    const std::optional<std::string>& /*aclTableName*/) {
  auto bcmSwitch = static_cast<const BcmSwitch*>(hwSwitch);
  auto defaultGroupID =
      bcmSwitch->getPlatform()->getAsic()->getDefaultACLGroupID();

  const auto stats = bcmSwitch->getStatUpdater()->getHwTableStats();
  ASSERT_EQ(aclCount, *stats.acl_entries_used());
  ASSERT_EQ(
      aclCount, fpGroupNumAclEntries(bcmSwitch->getUnit(), defaultGroupID));

  ASSERT_EQ(
      aclStatCount,
      BcmAclStat::getNumAclStatsInFpGroup(bcmSwitch, defaultGroupID));
  ASSERT_EQ(
      counterCount, bcmSwitch->getStatUpdater()->getAclStatCounterCount());
}

void checkAclStat(
    const HwSwitch* hwSwitch,
    std::shared_ptr<SwitchState> state,
    std::vector<std::string> acls,
    const std::string& statName,
    std::vector<cfg::CounterType> counterTypes,
    const std::optional<std::string>& /*aclTableName*/) {
  auto bcmSwitch = static_cast<const BcmSwitch*>(hwSwitch);
  auto aclTable = bcmSwitch->getAclTable();

  // Check if the stat has been programmed
  auto hwStat = aclTable->getAclStat(statName);
  ASSERT_NE(nullptr, hwStat);

  // Check the ACL table refcount
  ASSERT_EQ(acls.size(), aclTable->getAclStatRefCount(statName));

  // Check that the SW and HW configs are the same
  for (const auto& aclName : acls) {
    auto swTrafficCounter = getAclTrafficCounter(state, aclName);
    ASSERT_TRUE(swTrafficCounter);
    ASSERT_EQ(statName, *swTrafficCounter->name());
    ASSERT_EQ(counterTypes, *swTrafficCounter->types());
    BcmAclStat::isStateSame(
        bcmSwitch, hwStat->getHandle(), swTrafficCounter.value());
  }

  // Check the Stat Updater
  for (auto type : counterTypes) {
    ASSERT_NE(
        nullptr,
        bcmSwitch->getStatUpdater()->getAclStatCounterIf(
            hwStat->getHandle(), type, hwStat->getActionIndex()));
  }
}

void checkAclStatDeleted(
    const HwSwitch* hwSwitch,
    const std::string& statName) {
  auto bcmSwitch = static_cast<const BcmSwitch*>(hwSwitch);

  ASSERT_EQ(nullptr, bcmSwitch->getAclTable()->getAclStatIf(statName));
}

void checkAclStatSize(const HwSwitch* hwSwitch, const std::string& statName) {
  /*
   * Check the assumptions made in BcmAclStat::isStateSame()
   * See BcmAclStat.cpp for more info.
   *
   * Check that wedge40's asic is indeed programming *all* the types of
   * counters when asked to program only a subset.
   */
  auto bcmSwitch = static_cast<const BcmSwitch*>(hwSwitch);

  // We only programmed a packet counter
  int expectedNumCounters = 1;

  auto hwStat = bcmSwitch->getAclTable()->getAclStat(statName);
  int numCounters;
  if (hwSwitch->getPlatform()->getAsic()->isSupported(
          HwAsic::Feature::INGRESS_FIELD_PROCESSOR_FLEX_COUNTER)) {
    numCounters = BcmIngressFieldProcessorFlexCounter::getCounterTypeList(
                      bcmSwitch->getUnit(), hwStat->getHandle())
                      .size();
    // We also program both bytes and packets counter in one flex counter.
    expectedNumCounters = 2;
  } else {
    auto rv = bcm_field_stat_size(
        bcmSwitch->getUnit(), hwStat->getHandle(), &numCounters);
    bcmCheckError(
        rv, "Unable to get stat size for acl stat=", hwStat->getHandle());
  }

  ASSERT_EQ(expectedNumCounters, numCounters);
}

uint64_t getAclCounterStats(
    const HwSwitch* hw,
    const std::string& statName,
    cfg::CounterType counterType) {
  std::vector<facebook::fboss::cfg::CounterType> kGenericCounter;
  bcm_field_stat_t bcmFieldType;
  if (counterType == cfg::CounterType::BYTES) {
    bcmFieldType = bcmFieldStatBytes;
    kGenericCounter = kByteCounters;
  } else {
    bcmFieldType = bcmFieldStatPackets;
    kGenericCounter = kPacketCounters;
  }
  auto bcmSwitch = static_cast<const BcmSwitch*>(hw);
  auto statHandle = bcmSwitch->getAclTable()->getAclStat(statName)->getHandle();

  uint64_t value;
  if (hw->getPlatform()->getAsic()->isSupported(
          HwAsic::Feature::INGRESS_FIELD_PROCESSOR_FLEX_COUNTER)) {
    auto actionIndex =
        bcmSwitch->getAclTable()->getAclStat(statName)->getActionIndex();
    const auto& stats =
        BcmIngressFieldProcessorFlexCounter::getAclTrafficFlexCounterStats(
            bcmSwitch->getUnit(), statHandle, actionIndex, kGenericCounter);
    value = stats.at(counterType);
  } else {
    bcm_field_stat_t type = bcmFieldType;
    auto rv =
        bcm_field_stat_sync_get(bcmSwitch->getUnit(), statHandle, type, &value);
    bcmCheckError(rv, "Failed to update stat=", statHandle);
  }
  return value;
}

uint64_t getAclInOutBytes(
    const HwSwitch* hw,
    std::shared_ptr<SwitchState> /*state*/,
    const std::string& /*aclName*/,
    const std::string& statName,
    cfg::AclStage /* aclStage */,
    const std::optional<std::string>& /*aclTableName*/) {
  return getAclCounterStats(hw, statName, cfg::CounterType::BYTES);
}

uint64_t getAclInOutPackets(
    const HwSwitch* hw,
    std::shared_ptr<SwitchState> /*state*/,
    const std::string& /*aclName*/,
    const std::string& statName,
    cfg::AclStage /* aclStage */,
    const std::optional<std::string>& /*aclTableName*/) {
  return getAclCounterStats(hw, statName, cfg::CounterType::PACKETS);
}

} // namespace facebook::fboss::utility
