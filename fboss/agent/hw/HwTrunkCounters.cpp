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
  updateCounter(now, kInBytes(), *stats.inBytes_());
  updateCounter(now, kInUnicastPkts(), *stats.inUnicastPkts_());
  updateCounter(now, kInMulticastPkts(), *stats.inMulticastPkts_());
  updateCounter(now, kInBroadcastPkts(), *stats.inBroadcastPkts_());
  updateCounter(now, kInDiscards(), *stats.inDiscards_());
  updateCounter(now, kInErrors(), *stats.inErrors_());
  updateCounter(now, kInPause(), *stats.inPause_());
  updateCounter(now, kInIpv4HdrErrors(), *stats.inIpv4HdrErrors_());
  updateCounter(now, kInIpv6HdrErrors(), *stats.inIpv6HdrErrors_());

  updateCounter(now, kOutBytes(), *stats.outBytes_());
  updateCounter(now, kOutUnicastPkts(), *stats.outUnicastPkts_());
  updateCounter(now, kOutMulticastPkts(), *stats.outMulticastPkts_());
  updateCounter(now, kOutBroadcastPkts(), *stats.outBroadcastPkts_());
  updateCounter(now, kOutDiscards(), *stats.outDiscards_());
  updateCounter(now, kOutErrors(), *stats.outErrors_());
  updateCounter(now, kOutPause(), *stats.outPause_());
  updateCounter(
      now, kOutCongestionDiscards(), *stats.outCongestionDiscardPkts_());
  updateCounter(now, kOutEcnCounter(), *stats.outEcnCounter_());
  stats_ = stats;
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
  stats.inBytes_() = 0;
  stats.inUnicastPkts_() = 0;
  stats.inMulticastPkts_() = 0;
  stats.inBroadcastPkts_() = 0;
  stats.inDiscards_() = 0;
  stats.inErrors_() = 0;
  stats.inPause_() = 0;
  stats.inIpv4HdrErrors_() = 0;
  stats.inIpv6HdrErrors_() = 0;
  stats.inDiscardsRaw_() = 0;
  stats.inDstNullDiscards_() = 0;

  stats.outBytes_() = 0;
  stats.outUnicastPkts_() = 0;
  stats.outMulticastPkts_() = 0;
  stats.outBroadcastPkts_() = 0;
  stats.outDiscards_() = 0;
  stats.outErrors_() = 0;
  stats.outPause_() = 0;
  stats.outCongestionDiscardPkts_() = 0;
}

void accumulateHwTrunkMemberStats(
    HwTrunkStats& cumulativeSum,
    const HwPortStats& memberStats) {
  *cumulativeSum.inBytes_() += *memberStats.inBytes_();
  *cumulativeSum.inUnicastPkts_() += *memberStats.inUnicastPkts_();
  *cumulativeSum.inMulticastPkts_() += *memberStats.inMulticastPkts_();
  *cumulativeSum.inBroadcastPkts_() += *memberStats.inBroadcastPkts_();
  *cumulativeSum.inDiscards_() += *memberStats.inDiscards_();
  *cumulativeSum.inErrors_() += *memberStats.inErrors_();
  *cumulativeSum.inPause_() += *memberStats.inPause_();
  *cumulativeSum.inIpv4HdrErrors_() += *memberStats.inIpv4HdrErrors_();
  *cumulativeSum.inIpv6HdrErrors_() += *memberStats.inIpv6HdrErrors_();
  *cumulativeSum.inDiscardsRaw_() += *memberStats.inDiscardsRaw_();
  *cumulativeSum.inDstNullDiscards_() += *memberStats.inDstNullDiscards_();

  *cumulativeSum.outBytes_() += *memberStats.outBytes_();
  *cumulativeSum.outUnicastPkts_() += *memberStats.outUnicastPkts_();
  *cumulativeSum.outMulticastPkts_() += *memberStats.outMulticastPkts_();
  *cumulativeSum.outBroadcastPkts_() += *memberStats.outBroadcastPkts_();
  *cumulativeSum.outDiscards_() += *memberStats.outDiscards_();
  *cumulativeSum.outErrors_() += *memberStats.outErrors_();
  *cumulativeSum.outPause_() += *memberStats.outPause_();
  *cumulativeSum.outCongestionDiscardPkts_() +=
      *memberStats.outCongestionDiscardPkts_();
}

} // namespace facebook::fboss::utility
