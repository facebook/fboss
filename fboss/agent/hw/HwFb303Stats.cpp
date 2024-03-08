/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/HwFb303Stats.h"

#include "fboss/agent/hw/CounterUtils.h"

#include <folly/logging/xlog.h>

namespace facebook::fboss {

HwFb303Stats::~HwFb303Stats() {
  for (const auto& statNameAndStat : counters_) {
    utility::deleteCounter(statNameAndStat.second.fb303Counter.getName());
  }
}

const stats::MonotonicCounter* HwFb303Stats::getCounterIf(
    const std::string& statName) const {
  auto pcitr = counters_.find(statName);
  return pcitr != counters_.end() ? &pcitr->second.fb303Counter : nullptr;
}

stats::MonotonicCounter* HwFb303Stats::getCounterIf(
    const std::string& statName) {
  return const_cast<stats::MonotonicCounter*>(
      const_cast<const HwFb303Stats*>(this)->getCounterIf(statName));
}

int64_t HwFb303Stats::getCounterLastIncrement(
    const std::string& statName,
    std::optional<int64_t> defaultVal) const {
  auto stat = getCounterIf(statName);
  if (stat) {
    return stat->get();
  }
  if (defaultVal) {
    return *defaultVal;
  }
  throw FbossError(statName, " not found and no default value provided");
}

/*
 * Reinit port or port queue stat
 */
void HwFb303Stats::reinitStat(
    const std::string& statName,
    std::optional<std::string> oldStatName) {
  if (oldStatName) {
    if (oldStatName == statName) {
      return;
    }
    auto stat = getCounterIf(*oldStatName);
    stats::MonotonicCounter newStat{
        getMonotonicCounterName(statName), fb303::SUM, fb303::RATE};
    stat->swap(newStat);
    utility::deleteCounter(newStat.getName());
    counters_.insert(
        std::make_pair(statName, HwFb303Counter(std::move(*stat))));
    counters_.erase(*oldStatName);
  } else {
    counters_.emplace(
        statName,
        HwFb303Counter(stats::MonotonicCounter(
            getMonotonicCounterName(statName), fb303::SUM, fb303::RATE)));
  }
}

void HwFb303Stats::removeStat(const std::string& statName) {
  auto stat = getCounterIf(statName);
  if (stat == nullptr) {
    XLOG(ERR) << "Counter with " << statName << " missing";
    return;
  }
  utility::deleteCounter(getMonotonicCounterName(stat->getName()));
  counters_.erase(stat->getName());
}

void HwFb303Stats::updateStat(
    const std::chrono::seconds& now,
    const std::string& statName,
    int64_t val) {
  auto stat = getCounterIf(statName);
  CHECK(stat);
  stat->updateValue(now, val);
  auto pcitr = counters_.find(statName);
  CHECK(pcitr != counters_.end());
  pcitr->second.cumulativeValue = val;
}

const std::string HwFb303Stats::getMonotonicCounterName(
    const std::string& statName) const {
  return folly::to<std::string>(
      multiSwitchStatsPrefix_ ? *multiSwitchStatsPrefix_ : "", statName);
}

uint64_t HwFb303Stats::getCumulativeValueIf(const std::string& statName) const {
  auto pcitr = counters_.find(statName);
  return pcitr != counters_.end() ? pcitr->second.cumulativeValue : 0;
}

} // namespace facebook::fboss
