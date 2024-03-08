// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/test/utils/LoadBalancerTestUtils.h"

namespace {
constexpr uint8_t kDefaultQueue = 0;
}

namespace facebook::fboss::utility {

template <typename PortIdT, typename PortStatsT>
bool isLoadBalancedImpl(
    const std::map<PortIdT, PortStatsT>& portIdToStats,
    const std::vector<NextHopWeight>& weights,
    int maxDeviationPct,
    bool noTrafficOk) {
  auto ecmpPorts = folly::gen::from(portIdToStats) |
      folly::gen::map([](const auto& portIdAndStats) {
                     return portIdAndStats.first;
                   }) |
      folly::gen::as<std::vector<PortIdT>>();

  auto portBytes =
      folly::gen::from(portIdToStats) |
      folly::gen::map([&](const auto& portIdAndStats) {
        if constexpr (std::is_same_v<PortStatsT, HwPortStats>) {
          return *portIdAndStats.second.outBytes_();
        } else if constexpr (std::is_same_v<PortStatsT, HwSysPortStats>) {
          const auto& stats = portIdAndStats.second;
          return stats.queueOutBytes_()->at(kDefaultQueue) +
              stats.queueOutDiscardBytes_()->at(kDefaultQueue);
        }
        throw FbossError("Unsupported port stats type in isLoadBalancedImpl");
      }) |
      folly::gen::as<std::set<uint64_t>>();

  auto lowest = *portBytes.begin();
  auto highest = *portBytes.rbegin();
  XLOG(DBG0) << " Highest bytes: " << highest << " lowest bytes: " << lowest;
  if (!lowest) {
    return !highest && noTrafficOk;
  }
  if (!weights.empty()) {
    auto maxWeight = *(std::max_element(weights.begin(), weights.end()));
    for (auto i = 0; i < portIdToStats.size(); ++i) {
      uint64_t portOutBytes;
      if constexpr (std::is_same_v<PortStatsT, HwPortStats>) {
        portOutBytes = *portIdToStats.find(ecmpPorts[i])->second.outBytes_();
      } else if constexpr (std::is_same_v<PortStatsT, HwSysPortStats>) {
        const auto& stats = portIdToStats.find(ecmpPorts[i])->second;
        portOutBytes = stats.queueOutBytes_()->at(kDefaultQueue) +
            stats.queueOutDiscardBytes_()->at(kDefaultQueue);
      } else {
        throw FbossError("Unsupported port stats type in isLoadBalancedImpl");
      }
      auto weightPercent = (static_cast<float>(weights[i]) / maxWeight) * 100.0;
      auto portOutBytesPercent =
          (static_cast<float>(portOutBytes) / highest) * 100.0;
      auto percentDev = std::abs(weightPercent - portOutBytesPercent);
      // Don't tolerate a deviation of more than maxDeviationPct
      XLOG(DBG2) << "Percent Deviation: " << percentDev
                 << ", Maximum Deviation: " << maxDeviationPct;
      if (percentDev > maxDeviationPct) {
        return false;
      }
    }
  } else {
    auto percentDev = (static_cast<float>(highest - lowest) / lowest) * 100.0;
    // Don't tolerate a deviation of more than maxDeviationPct
    XLOG(DBG2) << "Percent Deviation: " << percentDev
               << ", Maximum Deviation: " << maxDeviationPct;
    if (percentDev > maxDeviationPct) {
      return false;
    }
  }
  return true;
}

template bool isLoadBalancedImpl<SystemPortID, HwSysPortStats>(
    const std::map<SystemPortID, HwSysPortStats>& portIdToStats,
    const std::vector<NextHopWeight>& weights,
    int maxDeviationPct,
    bool noTrafficOk);

template bool isLoadBalancedImpl<PortID, HwPortStats>(
    const std::map<PortID, HwPortStats>& portIdToStats,
    const std::vector<NextHopWeight>& weights,
    int maxDeviationPct,
    bool noTrafficOk);

template bool isLoadBalancedImpl<std::string, HwPortStats>(
    const std::map<std::string, HwPortStats>& portIdToStats,
    const std::vector<NextHopWeight>& weights,
    int maxDeviationPct,
    bool noTrafficOk);

template bool isLoadBalancedImpl<std::string, HwSysPortStats>(
    const std::map<std::string, HwSysPortStats>& portIdToStats,
    const std::vector<NextHopWeight>& weights,
    int maxDeviationPct,
    bool noTrafficOk);

} // namespace facebook::fboss::utility
