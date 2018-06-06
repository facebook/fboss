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
void BcmCosQueueManager::updateQueueAggregatedStat(
    const BcmCosQueueCounterType& /*type*/,
    facebook::stats::MonotonicCounter* /*counter*/,
    std::chrono::seconds /*now*/,
    HwPortStats* /*portStats*/) {}
}} // facebook::fboss
