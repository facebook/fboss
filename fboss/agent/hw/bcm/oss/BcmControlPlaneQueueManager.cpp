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
    opennsl_cos_queue_t /*cosQ*/) const {
  return std::shared_ptr<PortQueue>{};
}

void BcmControlPlaneQueueManager::program(
    const std::shared_ptr<PortQueue>& /*queue*/) {}

void BcmControlPlaneQueueManager::updateQueueStat(
    opennsl_cos_queue_t /*cosQ*/,
    const BcmCosQueueCounterType& /*type*/,
    facebook::stats::MonotonicCounter* /*counter*/,
    std::chrono::seconds /*now*/,
    HwPortStats* /*portStats*/) {}

void BcmControlPlaneQueueManager::getReservedBytes(
    opennsl_gport_t /*gport*/,
    int /*queueIdx*/,
    std::shared_ptr<PortQueue> /*queue*/) const {}
void BcmControlPlaneQueueManager::programReservedBytes(
    opennsl_gport_t /*gport*/,
    int /*queueIdx*/,
    const std::shared_ptr<PortQueue>& /*queue*/) {}
}} //facebook::fboss
