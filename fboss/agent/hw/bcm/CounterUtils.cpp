/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/bcm/CounterUtils.h"

#include "fboss/agent/hw/bcm/gen-cpp2/hardware_stats_constants.h"

#include <folly/logging/xlog.h>

namespace facebook {
namespace fboss {
namespace utility {

bool CounterPrevAndCur::counterUninitialized(const int64_t& val) const {
  return val == hardware_stats_constants::STAT_UNINITIALIZED();
}

int64_t getDerivedCounterIncrement(
    const CounterPrevAndCur& counterRaw,
    const std::vector<CounterPrevAndCur>& countersToSubtract) {
  auto constexpr kUninit = hardware_stats_constants::STAT_UNINITIALIZED();
  if (counterRaw.curUninitialized()) {
    XLOG(FATAL) << "Current counter values cannot be uninitialized";
  }
  if (counterRaw.prevUninitialized() || counterRaw.rolledOver()) {
    // If previous was uninitialized or counter rolled over return 0
    return 0;
  }
  auto increment = counterRaw.cur - counterRaw.prev;
  DCHECK_GE(increment, 0);
  for (const auto& counterToSub : countersToSubtract) {
    if (counterToSub.curUninitialized()) {
      XLOG(FATAL) << "Current counter values cannot be uninitialized";
    }
    if (counterToSub.prevUninitialized() || counterToSub.rolledOver()) {
      // If previous was uninitialized or counter rolled over return 0
      // If we ever need to be smarter here - we could also pass in bit
      // lengths of each of the above countersin h/w and then compute
      // exact increment given the prev and now rolled
      // over value.
      return 0;
    }
    increment -= (counterToSub.cur - counterToSub.prev);
  }
  // Since counters collection is not atomic, account for the fact
  // that counterToSubCur maybe > counterRawCur. Imagine a case
  // where all increments to raw counter are coming from counters
  // to subtract. Since counter syncing and collection is not
  // atomic, and assuming counterToSub were collected **after**
  // counterRawCur (its caller's responsibility to get this right)
  // we might get a < 0 value for increment
  return std::max(0L, increment);
}

int64_t getDerivedCounterIncrement(
    int64_t counterRawPrev,
    int64_t counterRawCur,
    int64_t counterToSubPrev,
    int64_t counterToSubCur) {
  return getDerivedCounterIncrement(
      CounterPrevAndCur{counterRawPrev, counterRawCur},
      {{CounterPrevAndCur{counterToSubPrev, counterToSubCur}}});
}
} // namespace utility
} // namespace fboss
} // namespace facebook
