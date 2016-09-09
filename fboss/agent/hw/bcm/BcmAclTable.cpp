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
#include "fboss/agent/hw/bcm/BcmSwitch.h"
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
  const auto& entry = aclEntryMap_.emplace(acl->getID(), std::move(bcmAcl));
  if (!entry.second) {
    throw FbossError("failed to add an existing acl entry");
  }
}

void BcmAclTable::processRemovedAcl(
  const std::shared_ptr<AclEntry>& acl) {
  // free the resources of acl in the reverse order of creation
  // remove the bcm acl entry first
  const auto numErasedAcl = aclEntryMap_.erase(acl->getID());
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

uint32_t BcmAclTable::getAclRangeCount() const {
  return aclRangeMap_.size();
}

BcmAclRange* BcmAclTable::incRefOrCreateBcmAclRange(const AclRange& range) {
  auto iter = aclRangeMap_.find(range);
  if (iter == aclRangeMap_.end()) {
    // if the range does not exist yet, create a new BcmAclRange
    BcmAclRange* r;
    std::unique_ptr<BcmAclRange> newRange =
      std::make_unique<BcmAclRange>(hw_, range);
    r = newRange.get();
    aclRangeMap_.emplace(range, std::make_pair(std::move(newRange), 1));
    return r;
  } else {
    // otherwise increase the reference count on the existing entry
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
