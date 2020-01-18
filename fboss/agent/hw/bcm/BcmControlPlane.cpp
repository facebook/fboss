/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/BcmControlPlane.h"

#include "fboss/agent/hw/bcm/BcmControlPlaneQueueManager.h"
#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmQosPolicyTable.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/state/PortQueue.h"

extern "C" {
#include <bcm/cosq.h>
#include <bcm/qos.h>
#include <bcm/types.h>
}

using facebook::stats::MonotonicCounter;
using std::chrono::duration_cast;
using std::chrono::seconds;
using std::chrono::system_clock;

namespace {
constexpr auto kCPUName = "cpu";
}

namespace facebook::fboss {

BcmControlPlane::BcmControlPlane(BcmSwitch* hw)
    : hw_(hw),
      gport_(BCM_GPORT_LOCAL_CPU),
      queueManager_(new BcmControlPlaneQueueManager(hw_, kCPUName, gport_)) {}

void BcmControlPlane::setupQueue(const PortQueue& queue) {
  queueManager_->program(queue);
}

void BcmControlPlane::setupRxReasonToQueue(
    const ControlPlane::RxReasonToQueue& /*reasonToQueue*/) {
  // TODO(joseph5wu) Implement the logic of setting reason mapping to bcm
}

void BcmControlPlane::setupIngressQosPolicy(
    const std::optional<std::string>& qosPolicyName) {
  int ingressHandle = 0; // 0 means 'detach the current policy'
  int egressHandle = -1; // -1 means do not change the current policy
  if (qosPolicyName) {
    auto qosPolicy = hw_->getQosPolicyTable()->getQosPolicy(*qosPolicyName);
    ingressHandle = qosPolicy->getHandle(BcmQosMap::Type::IP_INGRESS);
  }
  auto rv =
      bcm_qos_port_map_set(hw_->getUnit(), 0, ingressHandle, egressHandle);
  bcmCheckError(rv, "failed to set the qos map on CPU port");
}

void BcmControlPlane::updateQueueCounters() {
  auto now = duration_cast<seconds>(system_clock::now().time_since_epoch());
  queueManager_->updateQueueStats(now);
}

} // namespace facebook::fboss
