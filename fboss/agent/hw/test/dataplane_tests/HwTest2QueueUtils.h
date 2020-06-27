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
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/types.h"

#include <string>

/*
 * This utility is to provide utils for bcm 2 queue tests.
 */

namespace facebook::fboss::utility {

constexpr int k2QueueLowPriQueueId = 0;
constexpr int k2QueueHighPriQueueId = 1;
constexpr int k2QueueEcn1QueueId = 2;
constexpr int k2QueueNCQueueId = 7;

constexpr uint32_t k2QueueLowPriWeight = 10;
constexpr uint32_t k2QueueHighPriWeight = 90;
constexpr uint32_t k2QueueEcn1Weight = 8;

constexpr int k2QueueDefaultQueueId = k2QueueLowPriQueueId;
constexpr int k2QueueHighestSPQueueId = k2QueueNCQueueId;

void add2QueueConfig(cfg::SwitchConfig* config, PortID portID);
void add2QueueQosMaps(cfg::SwitchConfig& cfg);

std::string get2QueueCounterNameForDscp(uint8_t dscp);

const std::map<int, std::vector<uint8_t>>& k2QueueToDscp();
const std::map<int, uint8_t>& k2QueueWRRQueueToWeight();

const std::vector<int>& k2QueueWRRQueueIds();
const std::vector<int>& k2QueueSPQueueIds();
const std::vector<int>& k2QueueWRRAndNCQueueIds();

bool is2QueueWRRQueueId(int queueId);

} // namespace facebook::fboss::utility
