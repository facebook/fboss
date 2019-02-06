/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/BcmCosQueueManager.h"

namespace facebook { namespace fboss {
int BcmCosQueueManager::getControlValue(
    cfg::StreamType /*streamType*/,
    opennsl_gport_t /*gport*/,
    opennsl_cos_queue_t /*cosQ*/,
    BcmCosQueueControlType /*ctrlType*/) const {
  return 0;
}

void BcmCosQueueManager::programControlValue(
    cfg::StreamType /*streamType*/,
    opennsl_gport_t /*gport*/,
    opennsl_cos_queue_t /*cosQ*/,
    BcmCosQueueControlType /*ctrlType*/,
    int /*value*/) {}

void BcmCosQueueManager::getSchedulingAndWeight(
    opennsl_gport_t /*gport*/,
    opennsl_cos_queue_t /*cosQ*/,
    std::shared_ptr<PortQueue> /*queue*/) const {}

void BcmCosQueueManager::programSchedulingAndWeight(
    opennsl_gport_t /*gport*/,
    opennsl_cos_queue_t /*cosQ*/,
    const std::shared_ptr<PortQueue>& /*queue*/) {}

void BcmCosQueueManager::getReservedBytes(
    opennsl_gport_t /*gport*/,
    opennsl_cos_queue_t /*cosQ*/,
    std::shared_ptr<PortQueue> /*queue*/) const {}
void BcmCosQueueManager::programReservedBytes(
    opennsl_gport_t /*gport*/,
    int /*queueIdx*/,
    const std::shared_ptr<PortQueue>& /*queue*/) {}

void BcmCosQueueManager::getSharedBytes(
    opennsl_gport_t /*gport*/,
    opennsl_cos_queue_t /*cosQ*/,
    std::shared_ptr<PortQueue> /*queue*/) const {}
void BcmCosQueueManager::programSharedBytes(
    opennsl_gport_t /*gport*/,
    int /*queueIdx*/,
    const std::shared_ptr<PortQueue>& /*queue*/) {}

void BcmCosQueueManager::getBandwidth(
    opennsl_gport_t /*gport*/,
    int /*queueIdx*/,
    std::shared_ptr<PortQueue> /*queue*/) const {}
void BcmCosQueueManager::programBandwidth(
    opennsl_gport_t /*gport*/,
    opennsl_cos_queue_t /*cosQ*/,
    const std::shared_ptr<PortQueue>& /*queue*/) {}

void BcmCosQueueManager::updateQueueAggregatedStat(
    const BcmCosQueueCounterType& /*type*/,
    facebook::stats::MonotonicCounter* /*counter*/,
    std::chrono::seconds /*now*/,
    HwPortStats* /*portStats*/) {}
}} // facebook::fboss
