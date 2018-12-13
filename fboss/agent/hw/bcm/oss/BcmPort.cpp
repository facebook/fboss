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
#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"

extern "C" {
#include <opennsl/port.h>
}

namespace facebook { namespace fboss {

// stubbed out
void BcmPort::prepareForGracefulExit() {}
void BcmPort::setFEC(const std::shared_ptr<Port>& /*swPort*/) {}
void BcmPort::setPause(const std::shared_ptr<Port>& /*swPort*/) {}
void BcmPort::setTxSetting(const std::shared_ptr<Port>& /*swPort*/) {}
void BcmPort::setLoopbackMode(const std::shared_ptr<Port>& /*swPort*/) {}

bool BcmPort::isFECEnabled() {
  return false;
}

void BcmPort::setSflowRates(const std::shared_ptr<Port>& /* swPort */) {}
void BcmPort::disableSflow() {}

void BcmPort::setAdditionalStats(
    std::chrono::seconds /*now*/,
    HwPortStats* /*curPortStats*/) {}

bool BcmPort::shouldReportStats() const {
  return true;
}

cfg::PortSpeed BcmPort::getMaxSpeed() const {
  int speed;
  auto unit = hw_->getUnit();
  auto rv = opennsl_port_speed_max(hw_->getUnit(), port_, &speed);
  bcmCheckError(rv, "Failed to get max speed for port ", port_);
  return cfg::PortSpeed(speed);
}

opennsl_gport_t BcmPort::asGPort(opennsl_port_t /*port*/) {
  return static_cast<opennsl_gport_t>(0);
}

bool BcmPort::isValidLocalPort(opennsl_gport_t /*gport*/) {
  return false;
}

uint8_t BcmPort::determinePipe() const {
  return 0;
}

QueueConfig BcmPort::getCurrentQueueSettings() {
  return QueueConfig();
}

void BcmPort::setupQueue(const std::shared_ptr<PortQueue>& /*queue*/) {
}

void BcmPort::attachIngressQosPolicy(const std::string& /*name*/) {}

void BcmPort::detachIngressQosPolicy() {}

void BcmPort::setPortResource(const std::shared_ptr<Port>& /*swPort*/) {}

bool BcmPort::getDesiredFECEnabledStatus(
    const std::shared_ptr<Port>& /*swPort*/) {
  return false;
}

void BcmPort::updateBcmStats(std::chrono::seconds /*now*/,
                             HwPortStats* /*curPortStats*/) {
}
}} // namespace facebook::fboss
