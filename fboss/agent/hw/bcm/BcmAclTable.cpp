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
#include "fboss/agent/hw/bcm/BcmAclEntry.h"
#include "fboss/agent/hw/bcm/BcmAclRange.h"
#include "fboss/agent/hw/bcm/BcmStatUpdater.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/hw/bcm/BcmWarmBootCache.h"
#include "fboss/agent/hw/bcm/types.h"
#include "fboss/agent/FbossError.h"

#include <folly/CppAttributes.h>

namespace facebook { namespace fboss {

/*
 * Release all acl, stat and range entries.
 * Should only be called when we are about to reset/destroy the acl table
 */
void BcmAclTable::releaseAcls() {
  // AclEntries must be removed before the AclStats
  aclEntryMap_.clear();
  aclStatMap_.clear();
  aclRangeMap_.clear();
}

void BcmAclTable::processAddedAcl(
  const int groupId,
  const std::shared_ptr<AclEntry>& acl) {
  if (aclEntryMap_.find(acl->getPriority()) != aclEntryMap_.end()) {
    throw FbossError("ACL=", acl->getID(), " already exists");
  }

  std::unique_ptr<BcmAclEntry> bcmAcl =
    std::make_unique<BcmAclEntry>(hw_, groupId, acl);
  const auto& entry = aclEntryMap_.emplace(acl->getPriority(),
      std::move(bcmAcl));
  if (!entry.second) {
    throw FbossError("Failed to add an existing acl entry");
  }
}

void BcmAclTable::processRemovedAcl(
  const std::shared_ptr<AclEntry>& acl) {
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


BcmAclRange* FOLLY_NULLABLE BcmAclTable::getAclRangeIf(
  const AclRange& range) const {
  auto iter = aclRangeMap_.find(range);
  if (iter == aclRangeMap_.end()) {
    return nullptr;
  } else {
    return iter->second.first.get();
  }
}

BcmAclRange* BcmAclTable::getAclRange(const AclRange& range) const {
  auto bcmRange = getAclRangeIf(range);
  if (!bcmRange) {
    throw FbossError("Range: ", range, " does not exist");
  }
  return bcmRange;
}

uint32_t BcmAclTable::getAclRangeRefCount(const AclRange& range) const {
  auto iter = aclRangeMap_.find(range);
  if (iter == aclRangeMap_.end()) {
    return 0;
  } else {
    return iter->second.second;
  }
}

folly::Optional<uint32_t> BcmAclTable::getAclRangeRefCountIf(
    BcmAclRangeHandle handle) const {
  folly::Optional<uint32_t> refCount{folly::none};
  for (auto iter = aclRangeMap_.begin(); iter != aclRangeMap_.end(); iter++) {
    if (iter->second.first->getHandle() == handle) {
      refCount = iter->second.second;
      break;
    }
  }
  return refCount;
}

uint32_t BcmAclTable::getAclRangeCount() const {
  return aclRangeMap_.size();
}

BcmAclRange* BcmAclTable::incRefOrCreateBcmAclRange(const AclRange& range) {
  auto iter = aclRangeMap_.find(range);
  if (iter == aclRangeMap_.end()) {
    // If the range does not exist yet, create a new BcmAclRange
    auto newRange = std::make_unique<BcmAclRange>(hw_, range);
    auto r = newRange.get();
    aclRangeMap_.emplace(range, std::make_pair(std::move(newRange), 1));
    return r;
  } else {
    // Increase the reference count of the existing entry in BcmAclTable
    iter->second.second++;
    return iter->second.first.get();
  }
}

void BcmAclTable::derefBcmAclRange(const AclRange& range) {
  auto iter = aclRangeMap_.find(range);
  if (iter == aclRangeMap_.end()) {
    throw FbossError("decrease reference count on a non-existing BcmAclRange");
  }
  if (iter->second.second == 0) {
    throw FbossError("dereference a BcmAclRange whose reference is 0");
  }
  iter->second.second--;
  if (iter->second.second == 0) {
    aclRangeMap_.erase(iter);
  }
}

BcmAclStat* BcmAclTable::incRefOrCreateBcmAclStat(
  const std::string& counterName,
  BcmAclStatHandle statHandle) {
  auto aclStatItr = aclStatMap_.find(counterName);
  if (aclStatItr == aclStatMap_.end()) {
    auto newStat = std::make_unique<BcmAclStat>(hw_, statHandle);
    auto stat = newStat.get();
    aclStatMap_.emplace(counterName, std::make_pair(std::move(newStat), 1));
    hw_->getStatUpdater()->toBeAddedAclStat(stat->getHandle(), counterName);
    return stat;
  } else {
    CHECK(statHandle == aclStatItr->second.first->getHandle());
    aclStatItr->second.second++;
    return aclStatItr->second.first.get();
  }
}

BcmAclStat* BcmAclTable::incRefOrCreateBcmAclStat(
  const std::string& counterName,
  int gid) {
  auto aclStatItr = aclStatMap_.find(counterName);
  if (aclStatItr == aclStatMap_.end()) {
    auto newStat = std::make_unique<BcmAclStat>(hw_, gid);
    auto stat = newStat.get();
    aclStatMap_.emplace(counterName, std::make_pair(std::move(newStat), 1));
    hw_->getStatUpdater()->toBeAddedAclStat(stat->getHandle(), counterName);
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

BcmAclStat* FOLLY_NULLABLE BcmAclTable::getAclStatIf(
  const std::string& name) const {
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

}} // facebook::fboss
