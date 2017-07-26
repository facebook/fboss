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

#include "fboss/agent/state/AclEntry.h"
#include "fboss/agent/hw/bcm/BcmAclEntry.h"

#include <boost/container/flat_map.hpp>

namespace facebook { namespace fboss {

class BcmSwitch;

/**
 * A class to keep state related to acl entries in BcmSwitch
 */
class BcmAclTable {
 public:
  explicit BcmAclTable(BcmSwitch* hw) : hw_(hw) {}
  ~BcmAclTable() {}
  void processAddedAcl(const int groupId, const std::shared_ptr<AclEntry>& acl);
  void processRemovedAcl(const std::shared_ptr<AclEntry>& acl);

  // return nullptr if not found
  BcmAclRange*  getAclRangeIf(const AclRange& range) const;
  // return 0 if range does not exist
  uint32_t getAclRangeRefCount(const AclRange& range) const;
  uint32_t getAclRangeCount() const;

 private:
  BcmAclRange* incRefOrCreateBcmAclRange(const AclRange& range);
  BcmAclRange*  derefBcmAclRange(const AclRange& range);

  // map from acl range to bcm acl range and its reference count
  using BcmAclRangeMap = boost::container::flat_map<AclRange,
    std::pair<std::unique_ptr<BcmAclRange>, uint32_t>>;
  using BcmAclEntryMap = boost::container::flat_map<int,
    std::unique_ptr<BcmAclEntry>>;

  BcmSwitch* hw_;
  BcmAclRangeMap aclRangeMap_;
  BcmAclEntryMap aclEntryMap_;
};

}} // facebook::fboss
