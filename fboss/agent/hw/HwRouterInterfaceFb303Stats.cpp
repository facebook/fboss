// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/hw/HwRouterInterfaceFb303Stats.h"

namespace facebook::fboss {
namespace {
folly::StringPiece kInBytes() {
  return "inBytes";
}
folly::StringPiece kOutBytes() {
  return "outBytes";
}
folly::StringPiece kInErrorBytes() {
  return "inErrorBytes";
}
folly::StringPiece kOutErrorBytes() {
  return "outErrorBytes";
}

folly::StringPiece kInPkts() {
  return "inPkts";
}
folly::StringPiece kOutPkts() {
  return "outPkts";
}
folly::StringPiece kInErrorPkts() {
  return "inErrorPkts";
}
folly::StringPiece kOutErrorPkts() {
  return "outErrorPkts";
}
} // namespace
HwRouterInterfaceFb303Stats::HwRouterInterfaceFb303Stats(
    const std::string& interfaceName)
    : routerInterfaceName_(interfaceName), rifCounters_(routerInterfaceName_) {
  for (auto key : kRouterInterfaceMonotonicCounterStatKeys()) {
    rifCounters_.reinitStat(statName(key, routerInterfaceName_), std::nullopt);
  }
}

std::string HwRouterInterfaceFb303Stats::statName(
    folly::StringPiece statName,
    folly::StringPiece routerInterfaceName) {
  return folly::to<std::string>(routerInterfaceName, ".", statName);
}

void HwRouterInterfaceFb303Stats::clearStat(folly::StringPiece statKey) {
  rifCounters_.clearStat(statName(statKey, routerInterfaceName_));
}

void HwRouterInterfaceFb303Stats::updateStats(
    const HwRouterInterfaceStats& latestStats,
    const std::chrono::seconds& retrievedAt) {
  timeRetrieved_ = retrievedAt;

  updateStat(timeRetrieved_, kInBytes(), *latestStats.inBytes_());
  updateStat(timeRetrieved_, kOutBytes(), *latestStats.outBytes_());
  updateStat(timeRetrieved_, kInErrorBytes(), *latestStats.inErrorBytes_());
  updateStat(timeRetrieved_, kOutErrorBytes(), *latestStats.outErrorBytes_());
  updateStat(timeRetrieved_, kInPkts(), *latestStats.inPkts_());
  updateStat(timeRetrieved_, kOutPkts(), *latestStats.outPkts_());
  updateStat(timeRetrieved_, kInErrorPkts(), *latestStats.inErrorPkts_());
  updateStat(timeRetrieved_, kOutErrorPkts(), *latestStats.outErrorPkts_());
}

const std::vector<folly::StringPiece>&
HwRouterInterfaceFb303Stats::kRouterInterfaceMonotonicCounterStatKeys() {
  static std::vector<folly::StringPiece> keys = {
      kInBytes(),
      kInPkts(),
      kOutBytes(),
      kOutPkts(),
      kInErrorBytes(),
      kInErrorPkts(),
      kOutErrorBytes(),
      kOutErrorPkts()};
  return keys;
}

void HwRouterInterfaceFb303Stats::updateStat(
    const std::chrono::seconds& now,
    folly::StringPiece statKey,
    int64_t val) {
  rifCounters_.updateStat(now, statName(statKey, routerInterfaceName_), val);
}

} // namespace facebook::fboss
