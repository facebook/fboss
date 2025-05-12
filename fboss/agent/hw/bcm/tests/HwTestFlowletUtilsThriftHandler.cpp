// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/hw/test/HwTestThriftHandler.h"
#include "fboss/agent/state/RouteNextHopEntry.h"

namespace facebook::fboss::utility {

void HwTestThriftHandler::updateFlowletStats() {
  auto bcmSwitch = static_cast<const BcmSwitch*>(hwSwitch_);
  bcmSwitch->getHwFlowletStats();
  return;
}

cfg::SwitchingMode HwTestThriftHandler::getFwdSwitchingMode(
    std::unique_ptr<state::RouteNextHopEntry> routeNextHopEntry) {
  auto bcmSwitch = static_cast<BcmSwitch*>(hwSwitch_);
  auto rnhs = util::toRouteNextHopSet(*routeNextHopEntry->nexthops(), true);
  return bcmSwitch->getFwdSwitchingMode(
      RouteNextHopEntry(rnhs, *routeNextHopEntry->adminDistance()));
}

} // namespace facebook::fboss::utility
