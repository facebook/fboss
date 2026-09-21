// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/hw/HwFb303Stats.h"
#include "fboss/agent/hw/gen-cpp2/hardware_stats_types.h"

namespace facebook::fboss {

class HwRouterInterfaceFb303Stats {
 public:
  explicit HwRouterInterfaceFb303Stats(const std::string& interfaceName);

  ~HwRouterInterfaceFb303Stats() = default;

  const std::string& routerInterfaceName() const {
    return routerInterfaceName_;
  }

  static std::string statName(
      folly::StringPiece statName,
      folly::StringPiece routerInterfaceName);

  void clearStat(folly::StringPiece statKey);

  void updateStats(
      const HwRouterInterfaceStats& latestStats,
      const std::chrono::seconds& retrievedAt);

  static const std::vector<folly::StringPiece>&
  kRouterInterfaceMonotonicCounterStatKeys();

 private:
  void updateStat(
      const std::chrono::seconds& now,
      folly::StringPiece statKey,
      int64_t val);

  std::string routerInterfaceName_;
  HwFb303Stats rifCounters_;
  std::chrono::seconds timeRetrieved_{0};
};

} // namespace facebook::fboss
