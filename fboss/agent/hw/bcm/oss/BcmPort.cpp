/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/BcmPort.h"

namespace facebook { namespace fboss {

// stubbed out
void BcmPort::disablePause() {}
void BcmPort::setAdditionalStats(std::chrono::seconds now) {}
void BcmPort::setConfiguredMaxSpeed() {
  configuredMaxSpeed_ = cfg::PortSpeed::XG;
}
void BcmPort::setKR4Ability() {}
void BcmPort::remedy() {}
void BcmPort::prepareForGracefulExit() {}
void BcmPort::setFEC(const std::shared_ptr<Port>& swPort) {}

}} // namespace facebook::fboss
