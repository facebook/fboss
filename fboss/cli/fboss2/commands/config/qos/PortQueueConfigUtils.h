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

#include <string>
#include <utility>
#include <vector>

#include "fboss/agent/gen-cpp2/switch_config_types.h"

namespace facebook::fboss::utils {

// Applies queue configuration attributes onto `queue`. Shared by the
// `config qos default-queue-config` and `config qos queuing-policy <policy>`
// commands, which configure the same cfg::PortQueue attributes into different
// parts of the switch config.
//
// `attributes` is the flat list of scalar <attr, value> pairs (reserved-bytes,
// shared-bytes, weight, scaling-factor, scheduling, stream-type,
// buffer-pool-name); `aqmArgs` is the active-queue-management sub-arg stream.
//
// Merges onto the given queue: fields not named are left untouched, and an AQM
// edit targets the entry matching its congestion-behavior (appending a new one
// so ECN and EARLY_DROP entries coexist rather than clobbering aqms[0]).
//
// Throws std::invalid_argument on an empty edit (both lists empty) or any
// invalid, unknown, or duplicated attribute; the caller is expected to apply
// the mutation transactionally (edit a copy, splice back only on success).
void applyPortQueueConfig(
    cfg::PortQueue& queue,
    const std::vector<std::pair<std::string, std::string>>& attributes,
    const std::vector<std::string>& aqmArgs);

} // namespace facebook::fboss::utils
