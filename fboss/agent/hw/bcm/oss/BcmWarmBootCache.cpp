/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/BcmWarmBootCache.h"

#include "fboss/agent/hw/bcm/BcmAclEntry.h"
#include "fboss/agent/hw/bcm/BcmAclRange.h"

namespace facebook { namespace fboss {

void BcmWarmBootCache::populateAcls(
  const int /*groupId*/, AclRange2BcmAclRangeHandle& /*ranges*/,
  AclEntry2AclStat& /*stats*/,
  Priority2BcmAclEntryHandle& /*acls*/) {}

void BcmWarmBootCache::populateAclRanges(
  const BcmAclEntryHandle /*acl*/,
  AclRange2BcmAclRangeHandle& /*ranges*/) {}

void BcmWarmBootCache::populateAclStats(const BcmAclEntryHandle /*acl*/,
  AclEntry2AclStat& /*stats*/) {}

void BcmWarmBootCache::removeBcmAcl(BcmAclEntryHandle /*handle*/) {}

void BcmWarmBootCache::removeBcmAclRange(BcmAclRangeHandle /*handle*/) {}

void BcmWarmBootCache::removeBcmAclStat(BcmAclStatHandle /*handle*/) {}

void BcmWarmBootCache::detachBcmAclStat(BcmAclEntryHandle /*acl handle*/,
  BcmAclStatHandle /*stat handle*/) {}

}} // facebook::fboss
