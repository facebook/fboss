/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/StaticL2ForNeighborObserver.h"

#include "fboss/agent/StaticL2ForNeighborSwSwitchUpdater.h"
#include "fboss/agent/SwSwitch.h"
namespace facebook::fboss {

StaticL2ForNeighborObserver::StaticL2ForNeighborObserver(SwSwitch* sw)
    : sw_(sw) {
  sw_->registerStateObserver(this, "StaticL2ForNeighborObserver");
}
StaticL2ForNeighborObserver::~StaticL2ForNeighborObserver() {
  sw_->unregisterStateObserver(this);
}
void StaticL2ForNeighborObserver::stateUpdated(const StateDelta& stateDelta) {
  // The current StaticL2ForNeighborObserver logic relies on VLANs, MAC learning
  // callbacks etc. that are not supported on VOQ/Fabric switches. Thus, run
  // this stateObserver for NPU switches only.
  if (!sw_->getSwitchInfoTable().haveL3Switches() ||
      sw_->getSwitchInfoTable().l3SwitchType() != cfg::SwitchType::NPU) {
    return;
  }

  StaticL2ForNeighborSwSwitchUpdater updater(sw_);
  updater.stateUpdated(stateDelta);
}

} // namespace facebook::fboss
