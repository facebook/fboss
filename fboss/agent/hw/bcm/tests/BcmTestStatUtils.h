/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#pragma once

#include "fboss/agent/hw/bcm/types.h"
#include "fboss/agent/hw/gen-cpp2/hardware_stats_types.h"

extern "C" {
#include <bcm/stat.h>
#include <bcm/types.h>
}

namespace facebook::fboss::utility {

uint64_t getAclInOutPackets(int unit, BcmAclStatHandle handle);
uint64_t
getQueueOutPackets(int unit, bcm_gport_t gport, bcm_cos_queue_t cosq = 0);
uint64_t getQueueOutPackets(
    bool useQueueGportForCos,
    int unit,
    int port,
    int queueId,
    bool isMulticast);

void clearPortStats(int unit, bcm_port_t port);

std::map<int, uint64_t> clearAndGetQueueStats(
    int unit,
    bcm_port_t port,
    const std::vector<int>& queueIds);
} // namespace facebook::fboss::utility
