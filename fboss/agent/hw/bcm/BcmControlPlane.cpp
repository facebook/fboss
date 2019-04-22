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

extern "C" {
#include <opennsl/types.h>
}

namespace {
constexpr auto kCPUName = "cpu";
}

namespace facebook {
namespace fboss {

BcmControlPlane::BcmControlPlane(BcmSwitch* hw)
    : hw_(hw),
      gport_(OPENNSL_GPORT_LOCAL_CPU),
      queueManager_(new BcmControlPlaneQueueManager(hw_, kCPUName, gport_)) {}

void BcmControlPlane::setupQueue(const std::shared_ptr<PortQueue>& queue) {
  queueManager_->program(queue);
}

void BcmControlPlane::setupRxReasonToQueue(
    const ControlPlane::RxReasonToQueue& /*reasonToQueue*/) {
  // TODO(joseph5wu) Implement the logic of setting reason mapping to bcm
}

} // namespace fboss
} // namespace facebook
