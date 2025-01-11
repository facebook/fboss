// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/test/utils/StatsTestUtils.h"

namespace facebook::fboss {
std::string statsDelta(
    const multiswitch::HwSwitchStats& before,
    const multiswitch::HwSwitchStats& after) {
  std::stringstream ss;
  auto compareAndPrintMap =
      [&ss](const auto& before, const auto& after, const std::string& type) {
        if (before != after) {
          ss << type << " delta: " << statsMapDelta(before, after);
        }
      };
  compareAndPrintMap(*before.hwPortStats(), *after.hwPortStats(), "Port Stats");
  compareAndPrintMap(
      *before.hwTrunkStats(), *after.hwTrunkStats(), "Trunk Stats");
  compareAndPrintMap(
      *before.sysPortStats(), *after.sysPortStats(), "SysPort stats");

  auto compareAndPrint =
      [&ss](const auto& before, const auto& after, const std::string& type) {
        if (before != after) {
          ss << type << " delta: " << std::endl
             << " Before: " << before << std::endl
             << " After: " << after << std::endl;
        }
      };
  compareAndPrint(
      *before.hwResourceStats(), *after.hwResourceStats(), "Resource stats");
  compareAndPrint(*before.hwAsicErrors(), *after.hwAsicErrors(), "Asic Errors");
  compareAndPrint(*before.teFlowStats(), *after.teFlowStats(), "TEFlow stats");

  compareAndPrint(
      *before.fabricReachabilityStats(),
      *after.fabricReachabilityStats(),
      "Fabric reachability stats");
  compareAndPrint(
      *before.fb303GlobalStats(),
      *after.fb303GlobalStats(),
      "FB303 global stats");
  compareAndPrint(
      *before.cpuPortStats(), *after.cpuPortStats(), "CPU port stats");
  compareAndPrint(
      *before.switchDropStats(), *after.switchDropStats(), "Switch drop stats");
  compareAndPrint(
      *before.flowletStats(), *after.flowletStats(), "Flowlet stats");
  // TODO print delta for phyInfo, AclStats and HwSwitchWatermarkStats
  return ss.str();
}
} // namespace facebook::fboss
