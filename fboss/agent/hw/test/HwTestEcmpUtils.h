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

#include "fboss/agent/types.h"

#include <folly/IPAddress.h>

#include <set>

namespace facebook::fboss {
class HwSwitch;
namespace utility {

std::multiset<uint64_t> getEcmpMembersInHw(
    const facebook::fboss::HwSwitch* hw,
    const folly::CIDRNetwork& prefix,
    facebook::fboss::RouterID rid,
    int sizeInSw);

inline int getEcmpSizeInHw(
    const facebook::fboss::HwSwitch* hw,
    const folly::CIDRNetwork& prefix,
    facebook::fboss::RouterID rid,
    int sizeInSw) {
  return getEcmpMembersInHw(hw, prefix, rid, sizeInSw).size();
}
uint64_t getEcmpMemberWeight(
    const facebook::fboss::HwSwitch* hw,
    const std::multiset<uint64_t>& pathsInHw,
    uint64_t pathInHw);
uint64_t getTotalEcmpMemberWeight(
    const facebook::fboss::HwSwitch* hw,
    const std::multiset<uint64_t>& pathsInHw);
} // namespace utility
} // namespace facebook::fboss
