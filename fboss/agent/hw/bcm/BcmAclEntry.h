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

#include "fboss/agent/FbossError.h"
#include "fboss/agent/types.h"

namespace facebook { namespace fboss {

// We define the handle type for open source code
// The handle types should match the types defined by the switch sdk
using BcmAclRangeHandle = uint32_t;
using BcmAclEntryHandle = int;

class BcmSwitch;
class AclEntry;

/**
 * AclRange is the class that contains a range's value for range checker
 * Depending on the flags, the AclRange class can stand for a range of
 * different types (like src l4 port and dst l4 port)
 */
class AclRange {
public:
  explicit AclRange(const AclRange& r)
    : flags_(r.flags_), min_(r.min_), max_(r.max_) {
  }

  AclRange(AclRange&& r) noexcept
    : flags_(r.flags_), min_(r.min_), max_(r.max_) {
  }

  AclRange(const uint32_t flags, const uint32_t min, const uint32_t max)
    : flags_(flags), min_(min), max_(max) {
  }

  enum AclRangeFlags {
    SRC_L4_PORT = 0,
    DST_L4_PORT = 1,
    PKT_LEN     = 2,
  };

  bool operator<(const AclRange& r) const {
    return std::tie(flags_, min_, max_) < std::tie(r.flags_, r.min_, r.max_);
  }

  bool operator==(const AclRange& r) const {
    return std::tie(flags_, min_, max_) == std::tie(r.flags_, r.min_, r.max_);
  }

  AclRange& operator=(const AclRange& r) {
    std::tie(flags_, min_, max_) = std::tie(r.flags_, r.min_, r.max_);
    return *this;
  }

  uint32_t getFlags() const {
    return flags_;
  }

  void setFlags(const uint32_t flags) {
    flags_ = flags;
  }

  uint32_t getMin() const {
    return min_;
  }

  void setMin(const uint32_t min) {
    min_ = min;
  }

  uint32_t getMax() const {
    return max_;
  }

  void setMax(const uint32_t max) {
    max_ = max;
  }

  bool isExactMatch() const {
    return max_ == min_;
  }

  std::string str() const;

private:
  uint32_t flags_;
  uint32_t min_;
  uint32_t max_;
};

/**
 *  BcmAclRange is the class to abstract an range's resources and functions
 */
class BcmAclRange {
public:
  BcmAclRange(BcmSwitch* hw, const AclRange& range);
  ~BcmAclRange();
  BcmAclRangeHandle getHandle() const {
    return handle_;
  }
private:
  BcmSwitch* hw_;
  BcmAclRangeHandle handle_;
};

/**
 * BcmAclEntry is the class to abstract an acl's resources in BcmSwitch
 */
class BcmAclEntry {
public:

  using BcmAclRanges = std::vector<BcmAclRange *>;

  BcmAclEntry(BcmSwitch* hw, int gid,
    const std::shared_ptr<AclEntry>& acl,
    const BcmAclRanges& ranges);
  ~BcmAclEntry();
  BcmAclEntryHandle getHandle() const {
    return handle_;
  }
  /**
   * Check whether the acl details of handle in h/w matches the s/w acl and
   * ranges
   */
  static bool isStateSame(BcmSwitch* hw, int gid, BcmAclEntryHandle handle,
                          const std::shared_ptr<AclEntry>& acl);

private:
  BcmSwitch* hw_;
  int gid_;
  std::shared_ptr<AclEntry> acl_;
  BcmAclEntryHandle handle_;
};

}} // facebook::fboss
