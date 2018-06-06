/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/BcmControlPlaneQueueManager.h"

namespace facebook { namespace fboss {
BcmControlPlaneQueueManager::BcmControlPlaneQueueManager(
    BcmSwitch* hw,
    const std::string& portName,
    opennsl_gport_t portGport)
  : BcmCosQueueManager(hw, portName, portGport) {}

std::shared_ptr<PortQueue> BcmControlPlaneQueueManager::getCurrentQueueSettings(
    cfg::StreamType /*streamType*/,
    int /*queueIdx*/) const {
  return std::shared_ptr<PortQueue>{};
}

void BcmControlPlaneQueueManager::program(
    const std::shared_ptr<PortQueue>& /*queue*/) {}

void BcmControlPlaneQueueManager::updateQueueStat(
    int /*queueIdx*/,
    const BcmCosQueueCounterType& /*type*/,
    facebook::stats::MonotonicCounter* /*counter*/,
    std::chrono::seconds /*now*/,
    HwPortStats* /*portStats*/) {}
}} //facebook::fboss
