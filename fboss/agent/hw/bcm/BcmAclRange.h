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

#include <cstdint>
#include <string>
#include <tuple>

#include "fboss/agent/hw/bcm/types.h"

namespace facebook {
namespace fboss {

class BcmSwitch;

/**
 * AclRange is the class that contains a range's value for range checker
 * Depending on the flags, the AclRange class can stand for a range of
 * different types (like src l4 port and dst l4 port)
 */
class AclRange {
public:
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

  uint32_t getFlags() const {
    return flags_;
  }

  uint32_t getMin() const {
    return min_;
  }

  uint32_t getMax() const {
    return max_;
  }

  std::string str() const;

private:
  uint32_t flags_;
  uint32_t min_;
  uint32_t max_;
};

void toAppend(const AclRange& range, std::string* result);
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

}}
