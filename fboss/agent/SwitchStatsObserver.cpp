// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/SwitchStatsObserver.h"
#include "fboss/agent/SwSwitch.h"

namespace facebook::fboss {

SwitchStatsObserver::SwitchStatsObserver(SwSwitch* sw) : sw_(sw) {
  sw_->registerStateObserver(this, "SwitchStatsObserver");
}

SwitchStatsObserver::~SwitchStatsObserver() {
  sw_->unregisterStateObserver(this);
}

void SwitchStatsObserver::stateUpdated(const StateDelta& stateDelta) {}
} // namespace facebook::fboss
