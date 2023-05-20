// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/SwitchStatsObserver.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/SwitchStats.h"
#include "fboss/agent/state/DeltaFunctions.h"

namespace facebook::fboss {

SwitchStatsObserver::SwitchStatsObserver(SwSwitch* sw) : sw_(sw) {
  sw_->registerStateObserver(this, "SwitchStatsObserver");
}

SwitchStatsObserver::~SwitchStatsObserver() {
  sw_->unregisterStateObserver(this);
}

void SwitchStatsObserver::updateSystemPortCounters(
    const StateDelta& stateDelta) {
  auto getSysPortDelta = [](const auto& delta) {
    int cntDelta = 0;
    DeltaFunctions::forEachChanged(
        delta,
        [&](const auto& /*old*/,
            const auto& /*new*/) { /* Counter not changed */ },
        [&](const auto& /*added*/) { cntDelta++; },
        [&](const auto& /*removed*/) { cntDelta--; });
    return cntDelta;
  };
  sw_->stats()->localSystemPort(
      getSysPortDelta(stateDelta.getSystemPortsDelta()));
  sw_->stats()->remoteSystemPort(
      getSysPortDelta(stateDelta.getRemoteSystemPortsDelta()));
}

void SwitchStatsObserver::stateUpdated(const StateDelta& stateDelta) {
  updateSystemPortCounters(stateDelta);
}

} // namespace facebook::fboss
