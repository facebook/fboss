/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/CounterUtils.h"

#include <fb303/ServiceData.h>
#include "fboss/agent/hw/gen-cpp2/hardware_stats_constants.h"

#include <folly/logging/xlog.h>

#include <numeric>

namespace facebook::fboss::utility {

CounterPrevAndCur::CounterPrevAndCur(int64_t prev, int64_t cur)
    : prev_(prev), cur_(cur) {
  if (curUninitialized()) {
    // Current in reality should never be uninit. So this could be a FATAL.
    // But to be defensive, if this assumption is ever wrong, lets find
    // that out in a error message rather than crashing a portion of the fleet.
    XLOG(ERR) << "Current counter values cannot be uninitialized";
  }
}

int64_t CounterPrevAndCur::incrementFromPrev() const {
  // If any value was uninitialized or counter rolled over. We cannot compute
  // increment so return 0. Else compute increment
  return (anyUninitialized() || rolledOver()) ? 0 : cur_ - prev_;
}

bool CounterPrevAndCur::counterUninitialized(const int64_t& val) const {
  return val == hardware_stats_constants::STAT_UNINITIALIZED();
}

int64_t subtractIncrements(
    const CounterPrevAndCur& counterRaw,
    const std::vector<CounterPrevAndCur>& countersToSubtract) {
  auto increment = std::accumulate(
      begin(countersToSubtract),
      end(countersToSubtract),
      counterRaw.incrementFromPrev(),
      [](auto const& inc, const auto& counterToSub) {
        return inc - counterToSub.incrementFromPrev();
      });
  // Since counters collection is not atomic, account for the fact
  // that counterToSubCur maybe > counterRawCur. Imagine a case
  // where all increments to raw counter are coming from counters
  // to subtract. Since counter syncing and collection is not
  // atomic, and assuming counterToSub were collected **after**
  // counterRawCur (its caller's responsibility to get this right)
  // we might get a < 0 value for increment
  return std::max(0L, increment);
}

void deleteCounter(const std::string& oldCounterName) {
  // Once deleted, the counter will no longer be reported (by fb303)
  fb303::fbData->getStatMap()->unExportStatAll(oldCounterName);
  fb303::fbData->clearCounter(oldCounterName);
}

} // namespace facebook::fboss::utility
