/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/BcmAclTable.h"
#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/bcm/BcmAclEntry.h"
#include "fboss/agent/hw/bcm/BcmStatUpdater.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/hw/bcm/BcmWarmBootCache.h"
#include "fboss/agent/hw/bcm/types.h"

#include <folly/CppAttributes.h>

namespace facebook::fboss {

/*
 * Release all acl, stat entries.
 * Should only be called when we are about to reset/destroy the acl table
 */
void BcmAclTable::releaseAcls() {
  // AclEntries must be removed before the AclStats
  aclEntryMap_.clear();
  aclStatMap_.clear();
}

void BcmAclTable::processAddedAcl(
    const int groupId,
    const std::shared_ptr<AclEntry>& acl) {
  if (aclEntryMap_.find(acl->getPriority()) != aclEntryMap_.end()) {
    throw FbossError("ACL=", acl->getID(), " already exists");
  }

  std::unique_ptr<BcmAclEntry> bcmAcl =
      std::make_unique<BcmAclEntry>(hw_, groupId, acl);
  const auto& entry =
      aclEntryMap_.emplace(acl->getPriority(), std::move(bcmAcl));
  if (!entry.second) {
    throw FbossError("Failed to add an existing acl entry");
  }
}

void BcmAclTable::processRemovedAcl(const std::shared_ptr<AclEntry>& acl) {
  const auto numErasedAcl = aclEntryMap_.erase(acl->getPriority());
  if (numErasedAcl == 0) {
    throw FbossError("Failed to erase an existing bcm acl entry");
  }
}

BcmAclEntry* FOLLY_NULLABLE BcmAclTable::getAclIf(int priority) const {
  auto iter = aclEntryMap_.find(priority);
  if (iter == aclEntryMap_.end()) {
    return nullptr;
  }
  return iter->second.get();
}

BcmAclEntry* BcmAclTable::getAcl(int priority) const {
  auto acl = getAclIf(priority);
  if (!acl) {
    throw FbossError("Acl with priority: ", priority, " does not exist");
  }
  return acl;
}

BcmAclStat* BcmAclTable::incRefOrCreateBcmAclStat(
    const std::string& counterName,
    const std::vector<cfg::CounterType>& counterTypes,
    BcmAclStatHandle statHandle) {
  auto aclStatItr = aclStatMap_.find(counterName);
  if (aclStatItr == aclStatMap_.end()) {
    auto newStat = std::make_unique<BcmAclStat>(hw_, statHandle);
    auto stat = newStat.get();
    aclStatMap_.emplace(counterName, std::make_pair(std::move(newStat), 1));
    hw_->getStatUpdater()->toBeAddedAclStat(
        stat->getHandle(), counterName, counterTypes);
    return stat;
  } else {
    CHECK(statHandle == aclStatItr->second.first->getHandle());
    aclStatItr->second.second++;
    return aclStatItr->second.first.get();
  }
}

BcmAclStat* BcmAclTable::incRefOrCreateBcmAclStat(
    const std::string& counterName,
    const std::vector<cfg::CounterType>& counterTypes,
    int gid) {
  auto aclStatItr = aclStatMap_.find(counterName);
  if (aclStatItr == aclStatMap_.end()) {
    auto newStat = std::make_unique<BcmAclStat>(hw_, gid, counterTypes);
    auto stat = newStat.get();
    aclStatMap_.emplace(counterName, std::make_pair(std::move(newStat), 1));
    hw_->getStatUpdater()->toBeAddedAclStat(
        stat->getHandle(), counterName, counterTypes);
    return stat;
  } else {
    aclStatItr->second.second++;
    return aclStatItr->second.first.get();
  }
}

void BcmAclTable::derefBcmAclStat(const std::string& counterName) {
  auto aclStatItr = aclStatMap_.find(counterName);
  if (aclStatItr == aclStatMap_.end()) {
    throw FbossError("Tried to delete a non-existent Acl stat: ", counterName);
  }
  auto bcmAclStat = aclStatItr->second.first.get();
  auto& aclStatRefCnt = aclStatItr->second.second;
  aclStatRefCnt--;
  if (!aclStatRefCnt) {
    hw_->getStatUpdater()->toBeRemovedAclStat(bcmAclStat->getHandle());
    aclStatMap_.erase(aclStatItr);
  }
}

BcmAclStat* FOLLY_NULLABLE
BcmAclTable::getAclStatIf(const std::string& name) const {
  auto iter = aclStatMap_.find(name);
  if (iter == aclStatMap_.end()) {
    return nullptr;
  } else {
    return iter->second.first.get();
  }
}

BcmAclStat* BcmAclTable::getAclStat(const std::string& stat) const {
  auto bcmStat = getAclStatIf(stat);
  if (!bcmStat) {
    throw FbossError("Stat: ", stat, " does not exist");
  }
  return bcmStat;
}

uint32_t BcmAclTable::getAclStatRefCount(const std::string& name) const {
  auto iter = aclStatMap_.find(name);
  if (iter == aclStatMap_.end()) {
    return 0;
  } else {
    return iter->second.second;
  }
}

uint32_t BcmAclTable::getAclStatCount() const {
  return aclStatMap_.size();
}

void BcmAclTable::forFilteredEach(Filter predicate, FilterAction action) const {
  auto iterator = FilterIterator(aclEntryMap_, predicate);
  std::for_each(iterator.begin(), iterator.end(), action);
}

} // namespace facebook::fboss
