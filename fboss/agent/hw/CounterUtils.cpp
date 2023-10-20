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

void deleteCounter(const folly::StringPiece oldCounterName) {
  // Once deleted, the counter will no longer be reported (by fb303)
  fb303::fbData->getStatMap()->unExportStatAll(oldCounterName);
  fb303::fbData->clearCounter(oldCounterName);
}

std::string counterTypeToString(cfg::CounterType type) {
  switch (type) {
    case cfg::CounterType::PACKETS:
      return "packets";
    case cfg::CounterType::BYTES:
      return "bytes";
  }
  throw std::runtime_error("Unsupported Counter Type");
}

std::string statNameFromCounterType(
    const std::string& statPrefix,
    cfg::CounterType counterType,
    std::optional<std::string> multiSwitchPrefix) {
  return folly::to<std::string>(
      multiSwitchPrefix ? *multiSwitchPrefix : "",
      statPrefix,
      ".",
      counterTypeToString(counterType));
}

ExportedCounter::ExportedCounter(
    const folly::StringPiece name,
    const folly::Range<const fb303::ExportType*>& exportTypes)
    : stat_(fb303::fbData->getStatMap()->getLockableStatNoExport(name)),
      name_(name),
      exportTypes_(exportTypes) {
  exportStat(name);
}

ExportedCounter::~ExportedCounter(void) {
  unexport();
}

ExportedCounter::ExportedCounter(ExportedCounter&& other) noexcept
    : stat_(std::move(other.stat_)),
      name_(std::move(other.name_)),
      exportTypes_(std::move(other.exportTypes_)) {}

ExportedCounter& ExportedCounter::operator=(ExportedCounter&& other) {
  stat_ = std::move(other.stat_);
  name_ = std::move(other.name_);
  exportTypes_ = std::move(other.exportTypes_);

  return *this;
}

void ExportedCounter::exportStat(const folly::StringPiece name) {
  if (name.empty()) {
    return;
  }

  for (auto exportType : exportTypes_) {
    fb303::fbData->getStatMap()->exportStat(name, exportType);
  }
}

void ExportedCounter::unexport() {
  if (!name_.empty()) {
    deleteCounter(name_);
    name_.clear();
  }
}

void ExportedCounter::setName(const folly::StringPiece name) {
  if (name == name_) {
    return;
  }

  // The only way to change export name is to create a new stat. We then
  // swap the new counter with our counter so the new stat with new name
  // now exports our counter at stat_. newStat now has an empty counter
  // exported as old name_. We then delete the old name_ and let newStat
  // go out of scope.
  //
  // If name is empty, we set new stat to old stat so the swap is a no-op.
  // We don't export the new empty name but we do unexport the old name.
  // So the counter is now unexported.
  auto newStat = stat_;
  if (!name.empty()) {
    newStat = fb303::fbData->getStatMap()->getLockableStatNoExport(name);
  }
  stat_.swap(newStat);
  exportStat(name);
  unexport();

  name_ = name;
}

void ExportedCounter::incrementValue(std::chrono::seconds now, int64_t amount) {
  stat_.lock()->addValue(now.count(), amount);
}

} // namespace facebook::fboss::utility
