/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/switch_asics/HwAsic.h"
#pragma once
#include "fboss/agent/gen-cpp2/switch_config_types.h"

#include <string>

/*
 * This utility is to provide utils for network ai hw tests.
 */

namespace facebook::fboss::utility {

enum class NetworkAIQueueType { DEFAULT, MONITORING, RDMA, NC };

/* network AI Qos queues*/
constexpr int kNetworkAIMonitoringQueueId = 6;
constexpr int kNetworkAIRdmaQueueId = 2;
constexpr int kNetworkAINCQueueId = 7;
constexpr int kNetworkAIDefaultQueueId = 0;

constexpr int kNetworkAIHighestQueueId = kNetworkAINCQueueId;

constexpr int kNetworkAIRdmaWeight = 8;
constexpr int kNetworkAIMonitoringWeight = 15;
constexpr int kNetworkAINcWeight = 80;
constexpr int kNetworkAIDefaultWeight = 5;

const std::map<int, std::vector<uint8_t>> kNetworkAIQueueToDscp();

void addNetworkAIQueueConfig(
    cfg::SwitchConfig* config,
    cfg::StreamType streamType,
    cfg::QueueScheduling schedType,
    const HwAsic* hwAsic,
    bool addWredConfig = false,
    bool addEcnConfig = true,
    std::unordered_map<NetworkAIQueueType, cfg::QueueScheduling>
        schedTypeOverride = {});

void addNetworkAIQosMaps(
    cfg::SwitchConfig& cfg,
    const std::vector<const HwAsic*>& asics);

int getNetworkAIQueueId(NetworkAIQueueType queueType);

void addVoqAqmConfig(
    cfg::SwitchConfig* config,
    cfg::StreamType streamType,
    const HwAsic* asic,
    bool addWredConfig,
    bool addEcnConfig);

void addEventorVoqConfig(cfg::SwitchConfig* config, cfg::StreamType streamType);

const std::vector<int> kNetworkAISPQueueIds();
const std::map<int, uint8_t> kNetworkAIWRRQueueToWeight();
const std::vector<int> kNetworkAIWRRQueueIds();
const std::vector<int> kNetworkAIWRRAndICPQueueIds();
const std::vector<int> kNetworkAIWRRAndNCQueueIds();

} // namespace facebook::fboss::utility
