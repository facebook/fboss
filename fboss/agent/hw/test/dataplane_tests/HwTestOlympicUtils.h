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

#include <string>

/*
 * This utility is to provide utils for hw olympic tests.
 */

namespace facebook::fboss::utility {

constexpr int kOlympicSilverQueueId = 0;
constexpr int kOlympicGoldQueueId = 1;
constexpr int kOlympicEcn1QueueId = 2;
constexpr int kOlympicBronzeQueueId = 4;
constexpr int kOlympicICPQueueId = 6;
constexpr int kOlympicNCQueueId = 7;

constexpr uint32_t kOlympicSilverWeight = 15;
constexpr uint32_t kOlympicGoldWeight = 80;
constexpr uint32_t kOlympicEcn1Weight = 8;
constexpr uint32_t kOlympicBronzeWeight = 5;

constexpr int kOlympicDefaultQueueId = kOlympicSilverQueueId;
constexpr int kOlympicHighestSPQueueId = kOlympicNCQueueId;
constexpr int kOlympicHighestQueueId = kOlympicNCQueueId;

void addOlympicQueueConfig(
    cfg::SwitchConfig* config,
    cfg::StreamType streamType,
    bool addWredConfig = false);
void addOlympicQosMaps(cfg::SwitchConfig& cfg);

std::string getOlympicCounterNameForDscp(uint8_t dscp);

const std::map<int, std::vector<uint8_t>>& kOlympicQueueToDscp();
const std::map<int, uint8_t>& kOlympicWRRQueueToWeight();

const std::vector<int>& kOlympicWRRQueueIds();
const std::vector<int>& kOlympicSPQueueIds();
const std::vector<int>& kOlympicWRRAndICPQueueIds();
const std::vector<int>& kOlympicWRRAndNCQueueIds();

bool isOlympicWRRQueueId(int queueId);

int getMaxWeightWRRQueue(const std::map<int, uint8_t>& queueToWeight);

} // namespace facebook::fboss::utility
