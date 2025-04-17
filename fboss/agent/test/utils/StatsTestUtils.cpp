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
  compareAndPrintMap(*before.phyInfo(), *after.phyInfo(), "Phy Info");
  compareAndPrint(*before.aclStats(), *after.aclStats(), "Acl stats");
  compareAndPrint(
      *before.switchWatermarkStats(),
      *after.switchWatermarkStats(),
      "Switch watermark stats");
  return ss.str();
}

std::string statsDelta(
    const std::map<uint16_t, multiswitch::HwSwitchStats>& before,
    const std::map<uint16_t, multiswitch::HwSwitchStats>& after) {
  auto deltaPrinter = [](const multiswitch::HwSwitchStats& b,
                         const multiswitch::HwSwitchStats& a) {
    return statsDelta(b, a);
  };
  return statsMapDelta(before, after, deltaPrinter);
}
} // namespace facebook::fboss
