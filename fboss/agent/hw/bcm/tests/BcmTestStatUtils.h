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

std::pair<uint64_t, uint64_t> getQueueOutPacketsAndBytes(
    int unit,
    bcm_gport_t gport,
    bcm_cos_queue_t cosq = 0);
std::pair<uint64_t, uint64_t> getQueueOutPacketsAndBytes(
    bool useQueueGportForCos,
    int unit,
    int port,
    int queueId,
    bool isMulticast);

} // namespace facebook::fboss::utility
