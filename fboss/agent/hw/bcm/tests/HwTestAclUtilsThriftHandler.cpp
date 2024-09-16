// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/hw/test/HwTestThriftHandler.h"

#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"

#include "fboss/agent/hw/bcm/BcmAclTable.h"
#include "fboss/agent/hw/bcm/BcmStatUpdater.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/test/utils/AclTestUtils.h"

namespace facebook {
namespace fboss {
namespace utility {

int32_t HwTestThriftHandler::getAclTableNumAclEntries(
    std::unique_ptr<std::string> /*name*/) {
  auto bcmSwitch = static_cast<const BcmSwitch*>(hwSwitch_);

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

bool HwTestThriftHandler::isDefaultAclTableEnabled() {
  auto bcmSwitch = static_cast<const BcmSwitch*>(hwSwitch_);

  bcm_field_group_t gid =
      bcmSwitch->getPlatform()->getAsic()->getDefaultACLGroupID();
  int enable = -1;

  auto rv = bcm_field_group_enable_get(bcmSwitch->getUnit(), gid, &enable);
  bcmCheckError(rv, "failed to get field group enable status");
  CHECK(enable == 0 || enable == 1);
  return (enable == 1);
}

bool HwTestThriftHandler::isAclTableEnabled(
    std::unique_ptr<std::string> /*name*/) {
  return isDefaultAclTableEnabled();
}

int32_t HwTestThriftHandler::getDefaultAclTableNumAclEntries() {
  auto bcmSwitch = static_cast<const BcmSwitch*>(hwSwitch_);

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

bool HwTestThriftHandler::isStatProgrammedInDefaultAclTable(
    std::unique_ptr<std::vector<::std::string>> aclEntryNames,
    std::unique_ptr<std::string> counterName,
    std::unique_ptr<std::vector<cfg::CounterType>> types) {
  auto bcmSwitch = static_cast<const BcmSwitch*>(hwSwitch_);
  auto aclTable = bcmSwitch->getAclTable();

  // Check if the stat has been programmed
  auto hwStat = aclTable->getAclStat(*counterName);
  if (!hwStat) {
    return false;
  }

  // Check the ACL table refcount
  if (aclEntryNames->size() != aclTable->getAclStatRefCount(*counterName)) {
    return false;
  }

  auto state = hwSwitch_->getProgrammedState();
  // Check that the SW and HW configs are the same
  for (const auto& aclName : *aclEntryNames) {
    auto swTrafficCounter = getAclTrafficCounter(state, aclName);
    if (!swTrafficCounter || *swTrafficCounter->name() != *counterName) {
      return false;
    }
    if (*types != *swTrafficCounter->types()) {
      return false;
    }
    BcmAclStat::isStateSame(
        bcmSwitch, hwStat->getHandle(), swTrafficCounter.value());
  }

  // Check the Stat Updater
  for (auto type : *types) {
    if (!bcmSwitch->getStatUpdater()->getAclStatCounterIf(
            hwStat->getHandle(), type, hwStat->getActionIndex())) {
      return false;
    }
  }
  return true;
}

bool HwTestThriftHandler::isStatProgrammedInAclTable(
    std::unique_ptr<std::vector<::std::string>> aclEntryNames,
    std::unique_ptr<std::string> counterName,
    std::unique_ptr<std::vector<cfg::CounterType>> types,
    std::unique_ptr<std::string> /*tableName*/) {
  return isStatProgrammedInDefaultAclTable(
      std::move(aclEntryNames), std::move(counterName), std::move(types));
}

bool HwTestThriftHandler::isAclEntrySame(
    std::unique_ptr<state::AclEntryFields> aclEntry,
    std::unique_ptr<std::string> /* aclTableName */) {
  auto state = hwSwitch_->getProgrammedState();
  auto bcmSwitch = static_cast<const BcmSwitch*>(hwSwitch_);

  auto swAcl = std::make_shared<AclEntry>(*aclEntry);
  auto hwAcl = bcmSwitch->getAclTable()->getAclIf(aclEntry->priority().value());
  if (nullptr == hwAcl) {
    return false;
  }
  if (!BcmAclEntry::isStateSame(
          bcmSwitch,
          bcmSwitch->getPlatform()->getAsic()->getDefaultACLGroupID(),
          hwAcl->getHandle(),
          swAcl)) {
    return false;
  }
  return true;
}

bool HwTestThriftHandler::areAllAclEntriesEnabled() {
  return isDefaultAclTableEnabled();
}

} // namespace utility
} // namespace fboss
} // namespace facebook
