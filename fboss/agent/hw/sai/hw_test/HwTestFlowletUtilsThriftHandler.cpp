// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/sai/switch/SaiSwitch.h"
#include "fboss/agent/hw/test/HwTestThriftHandler.h"

namespace facebook::fboss::utility {

void HwTestThriftHandler::updateFlowletStats() {
  throw FbossError("Flowlet stats are not supported in SaiSwitch.");
}

cfg::SwitchingMode HwTestThriftHandler::getFwdSwitchingMode(
    std::unique_ptr<state::RouteNextHopEntry> routeNextHopEntry) {
  auto saiSwitch = static_cast<SaiSwitch*>(hwSwitch_);
  auto rnhs = util::toRouteNextHopSet(*routeNextHopEntry->nexthops(), true);
  return saiSwitch->getFwdSwitchingMode(
      RouteNextHopEntry(rnhs, *routeNextHopEntry->adminDistance()));
}

} // namespace facebook::fboss::utility
