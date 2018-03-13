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
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/hw/bcm/BcmWarmBootCache.h"
#include "fboss/agent/FbossError.h"

#include <folly/CppAttributes.h>

namespace facebook { namespace fboss {

void BcmAclTable::processAddedAcl(
  const int groupId,
  const std::shared_ptr<AclEntry>& acl) {
  // check if range exists
  BcmAclEntry::BcmAclRanges bcmRanges;
  if (acl->getSrcL4PortRange() &&
      !acl->getSrcL4PortRange().value().isExactMatch()) {
    AclL4PortRange r = acl->getSrcL4PortRange().value();
    AclRange range(AclRange::SRC_L4_PORT, r.getMin(), r.getMax());
    BcmAclRange* bcmRange = incRefOrCreateBcmAclRange(range);
    bcmRanges.push_back(bcmRange);
  }

  if (acl->getDstL4PortRange() &&
      !acl->getDstL4PortRange().value().isExactMatch()) {
    AclL4PortRange r = acl->getDstL4PortRange().value();
    AclRange range(AclRange::DST_L4_PORT, r.getMin(), r.getMax());
    BcmAclRange* bcmRange = incRefOrCreateBcmAclRange(range);
    bcmRanges.push_back(bcmRange);
  }

  if (acl->getPktLenRange()) {
    AclPktLenRange r = acl->getPktLenRange().value();
    AclRange range(AclRange::PKT_LEN, r.getMin(), r.getMax());
    BcmAclRange* bcmRange = incRefOrCreateBcmAclRange(range);
    bcmRanges.push_back(bcmRange);
  }

  // create the new bcm acl entry and add it to the table
  std::unique_ptr<BcmAclEntry> bcmAcl = std::make_unique<BcmAclEntry>(
    hw_, groupId, acl, bcmRanges);
  const auto& entry = aclEntryMap_.emplace(acl->getPriority(),
      std::move(bcmAcl));
  if (!entry.second) {
    throw FbossError("failed to add an existing acl entry");
  }
}

void BcmAclTable::processRemovedAcl(
  const std::shared_ptr<AclEntry>& acl) {
  // free the resources of acl in the reverse order of creation
  // remove the bcm acl entry first
  const auto numErasedAcl = aclEntryMap_.erase(acl->getPriority());
  if (numErasedAcl == 0) {
    throw FbossError("failed to erase an existing bcm acl entry");
  }

  // remove unused ranges
  if (acl->getSrcL4PortRange() &&
      !acl->getSrcL4PortRange().value().isExactMatch()) {
    AclL4PortRange r = acl->getSrcL4PortRange().value();
    AclRange range(AclRange::SRC_L4_PORT, r.getMin(), r.getMax());
    derefBcmAclRange(range);
  }
  if (acl->getDstL4PortRange() &&
      !acl->getDstL4PortRange().value().isExactMatch()) {
    AclL4PortRange r = acl->getDstL4PortRange().value();
    AclRange range(AclRange::DST_L4_PORT, r.getMin(), r.getMax());
    derefBcmAclRange(range);
  }
  if (acl->getPktLenRange()) {
    AclPktLenRange r = acl->getPktLenRange().value();
    AclRange range(AclRange::PKT_LEN, r.getMin(), r.getMax());
    derefBcmAclRange(range);
  }

}

BcmAclEntry* FOLLY_NULLABLE BcmAclTable::getAclIf(int priority) const {
  auto iter = aclEntryMap_.find(priority);
  if (iter == aclEntryMap_.end()) {
    return nullptr;
  }
  return iter->second.get();
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
    BcmAclRange* r;
    std::unique_ptr<BcmAclRange> newRange =
      std::make_unique<BcmAclRange>(hw_, range);
    r = newRange.get();
    aclRangeMap_.emplace(range, std::make_pair(std::move(newRange), 1));
    return r;
  } else {
    const auto warmBootCache = hw_->getWarmBootCache();
    auto warmbootItr = warmBootCache->findBcmAclRange(range);
    // If the range also exists in warmboot cache, call programmed() to decrease
    // the reference count in warmboot cache
    if (warmbootItr != warmBootCache->aclRange2BcmAclRangeHandle_end()) {
      warmBootCache->programmed(warmbootItr);
    }
    // Increase the reference count of the existing entry in BcmAclTable
    iter->second.second++;
    return iter->second.first.get();
  }
}

BcmAclRange* FOLLY_NULLABLE BcmAclTable::derefBcmAclRange(
  const AclRange& range) {
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
    return nullptr;
  }
  return iter->second.first.get();
}

}} // facebook::fboss
