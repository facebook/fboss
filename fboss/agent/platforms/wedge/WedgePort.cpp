/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/platforms/wedge/WedgePort.h"

namespace facebook { namespace fboss {

WedgePort::WedgePort(PortID id)
  : id_(id) {
}

void WedgePort::setBcmPort(BcmPort* port) {
  bcmPort_ = port;
}

/*
 * TODO: Not much code here yet.
 * For now, QSFP handling on wedge is currently managed by separate tool.
 * We need a little more time to sync up on OpenNSL APIs to get the LED
 * handling code open source.
 */

void WedgePort::preDisable(bool temporary) {
}

void WedgePort::postDisable(bool temporary) {
}

void WedgePort::preEnable() {
}

void WedgePort::postEnable() {
}

bool WedgePort::isMediaPresent() {
  return false;
}

void WedgePort::statusIndication(bool enabled, bool link,
                                 bool ingress, bool egress,
                                 bool discards, bool errors) {
  linkStatusChanged(link, enabled);
}

void WedgePort::linkSpeedChanged(const cfg::PortSpeed& speed) {
  if (!qsfp_) {
    LOG (INFO) << " No QSFP set for : " << id_ << " skip updating QSFP "
      <<" settings ";
    return;
  }
  // This should be resolved in BcmPort before calling
  if (speed == cfg::PortSpeed::DEFAULT) {
    LOG(ERROR) << "Unresolved speed: Unable to determine what qsfp settings " <<
      "are needed for port " << id_;
    return;
  }
  qsfp_->customizeTransceiver(speed);
}
}} // facebook::fboss
