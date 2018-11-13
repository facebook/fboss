/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/BcmTableStats.h"

namespace facebook { namespace fboss {

bool BcmHwTableStatManager::refreshLPMOnlyStats(BcmHwTableStats*) {
  return false;
}

bool BcmHwTableStatManager::refreshHwStatusStats(BcmHwTableStats*) {
  return false;
}

bool BcmHwTableStatManager::refreshLPMStats(BcmHwTableStats*) {
  return false;
}

bool BcmHwTableStatManager::refreshFPStats(BcmHwTableStats*) {
  return false;
}

void BcmHwTableStatManager::updateBcmStateChangeStats(
    facebook::fboss::StateDelta const&,
    facebook::fboss::BcmHwTableStats*) {}

void BcmHwTableStatManager::decrementBcmMirrorStat(
    const std::shared_ptr<Mirror>& /*removedMirror*/,
    BcmHwTableStats* /*stats*/) {}
void BcmHwTableStatManager::incrementBcmMirrorStat(
    const std::shared_ptr<Mirror>& /*addedMirror*/,
    BcmHwTableStats* /*stats*/) {}

void BcmHwTableStatManager::publish(BcmHwTableStats) const {}

void BcmHwTableStatManager::refresh(
    const StateDelta& /*delta*/,
    BcmHwTableStats* /*stats*/) {}
}}
