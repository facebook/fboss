// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include "fboss/agent/hw/HwTrunkCounters.h"

#include "fboss/agent/hw/CounterUtils.h"
#include "fboss/agent/hw/StatsConstants.h"

namespace facebook::fboss::utility {

HwTrunkCounters::HwTrunkCounters(
    AggregatePortID aggPortID,
    std::string trunkName)
    : aggregatePortID_(aggPortID), trunkName_(trunkName) {
  reinitialize(aggPortID, trunkName);
}

void HwTrunkCounters::reinitialize(
    AggregatePortID aggPortID,
    std::string trunkName) {
  aggregatePortID_ = aggPortID;
  trunkName_ = trunkName;
  initializeCounter(kInBytes());
  initializeCounter(kInUnicastPkts());
  initializeCounter(kInMulticastPkts());
  initializeCounter(kInBroadcastPkts());
  initializeCounter(kInDiscards());
  initializeCounter(kInDiscardsRaw());
  initializeCounter(kInErrors());
  initializeCounter(kInPause());
  initializeCounter(kInIpv4HdrErrors());
  initializeCounter(kInIpv6HdrErrors());
  initializeCounter(kInDstNullDiscards());

  initializeCounter(kOutBytes());
  initializeCounter(kOutUnicastPkts());
  initializeCounter(kOutMulticastPkts());
  initializeCounter(kOutBroadcastPkts());
  initializeCounter(kOutDiscards());
  initializeCounter(kOutErrors());
  initializeCounter(kOutPause());
  initializeCounter(kOutCongestionDiscards());
  initializeCounter(kOutEcnCounter());
}

void HwTrunkCounters::initializeCounter(folly::StringPiece counterKey) {
  auto oldCounter = getCounterIf(counterKey);

  auto externalCounterName = constructCounterName(counterKey);
  auto newCounter =
      stats::MonotonicCounter({externalCounterName, fb303::SUM, fb303::RATE});

  if (oldCounter) {
    oldCounter->swap(newCounter);
    utility::deleteCounter(newCounter.getName());
  } else {
    counters_.emplace(counterKey.str(), std::move(newCounter));
  }
}

stats::MonotonicCounter* FOLLY_NULLABLE
HwTrunkCounters::getCounterIf(folly::StringPiece counterKey) {
  auto it = counters_.find(counterKey.str());
  if (it == counters_.end()) {
    return nullptr;
  }

  return &(it->second);
}

std::string HwTrunkCounters::constructCounterName(
    folly::StringPiece counterKey) const {
  if (trunkName_ == "") {
    return folly::to<std::string>("po", aggregatePortID_, ".", counterKey);
  }
  return folly::to<std::string>(trunkName_, ".", counterKey);
}

void HwTrunkCounters::updateCounters(
    std::chrono::seconds now,
    const HwTrunkStats& stats) {
  updateCounter(now, kInBytes(), *stats.inBytes__ref());
  updateCounter(now, kInUnicastPkts(), *stats.inUnicastPkts__ref());
  updateCounter(now, kInMulticastPkts(), *stats.inMulticastPkts__ref());
  updateCounter(now, kInBroadcastPkts(), *stats.inBroadcastPkts__ref());
  updateCounter(now, kInDiscards(), *stats.inDiscards__ref());
  updateCounter(now, kInErrors(), *stats.inErrors__ref());
  updateCounter(now, kInPause(), *stats.inPause__ref());
  updateCounter(now, kInIpv4HdrErrors(), *stats.inIpv4HdrErrors__ref());
  updateCounter(now, kInIpv6HdrErrors(), *stats.inIpv6HdrErrors__ref());

  updateCounter(now, kOutBytes(), *stats.outBytes__ref());
  updateCounter(now, kOutUnicastPkts(), *stats.outUnicastPkts__ref());
  updateCounter(now, kOutMulticastPkts(), *stats.outMulticastPkts__ref());
  updateCounter(now, kOutBroadcastPkts(), *stats.outBroadcastPkts__ref());
  updateCounter(now, kOutDiscards(), *stats.outDiscards__ref());
  updateCounter(now, kOutErrors(), *stats.outErrors__ref());
  updateCounter(now, kOutPause(), *stats.outPause__ref());
  updateCounter(
      now, kOutCongestionDiscards(), *stats.outCongestionDiscardPkts__ref());
  updateCounter(now, kOutEcnCounter(), *stats.outEcnCounter__ref());
}

void HwTrunkCounters::updateCounter(
    std::chrono::seconds now,
    folly::StringPiece counterKey,
    int64_t value) {
  auto counter = getCounterIf(counterKey);
  if (!counter) {
    return;
  }
  counter->updateValue(now, value);
}

void clearHwTrunkStats(HwTrunkStats& stats) {
  stats.inBytes__ref() = 0;
  stats.inUnicastPkts__ref() = 0;
  stats.inMulticastPkts__ref() = 0;
  stats.inBroadcastPkts__ref() = 0;
  stats.inDiscards__ref() = 0;
  stats.inErrors__ref() = 0;
  stats.inPause__ref() = 0;
  stats.inIpv4HdrErrors__ref() = 0;
  stats.inIpv6HdrErrors__ref() = 0;
  stats.inDiscardsRaw__ref() = 0;
  stats.inDstNullDiscards__ref() = 0;

  stats.outBytes__ref() = 0;
  stats.outUnicastPkts__ref() = 0;
  stats.outMulticastPkts__ref() = 0;
  stats.outBroadcastPkts__ref() = 0;
  stats.outDiscards__ref() = 0;
  stats.outErrors__ref() = 0;
  stats.outPause__ref() = 0;
  stats.outCongestionDiscardPkts__ref() = 0;
}

} // namespace facebook::fboss::utility
