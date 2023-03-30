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

namespace facebook::fboss {
class HwAsic;
}

/*
 * This utility is to provide utils for hw olympic tests.
 */

namespace facebook::fboss::utility {

/* Olympic QoS queues */
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

constexpr int kOlympicHighestSPQueueId = kOlympicNCQueueId;

/* Olympic ALL SP QoS queues */
constexpr int kOlympicAllSPNCNFQueueId = 0;
constexpr int kOlympicAllSPBronzeQueueId = 1;
constexpr int kOlympicAllSPSilverQueueId = 2;
constexpr int kOlympicAllSPGoldQueueId = 3;
constexpr int kOlympicAllSPICPQueueId = 6;
constexpr int kOlympicAllSPNCQueueId = 7;

constexpr int kOlympicAllSPHighestSPQueueId = kOlympicAllSPNCQueueId;

/* Queue config params */
constexpr int kQueueConfigBurstSizeMinKb = 1;
constexpr int kQueueConfigBurstSizeMaxKb = 224;
constexpr int kQueueConfigAqmsEcnThresholdMinMax = 120000;
constexpr int kQueueConfigAqmsWredThresholdMinMax = 660000;
constexpr int kQueueConfigAqmsWredDropProbability = 100;

/* network AI Qos queues*/
constexpr int kNetworkAIMonitoringQueueId = 0;
constexpr int kNetworkAIRdmaQueueId = 6;
constexpr int kNetworkAINCQueueId = 7;

void addNetworkAIQueueConfig(
    cfg::SwitchConfig* config,
    cfg::StreamType streamType);

void addOlympicQueueConfig(
    cfg::SwitchConfig* config,
    cfg::StreamType streamType,
    const HwAsic* asic,
    bool addWredConfig = false);
void addQueueWredDropConfig(
    cfg::SwitchConfig* config,
    cfg::StreamType streamType,
    const HwAsic* asic);

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

void addOlympicAllSPQueueConfig(
    cfg::SwitchConfig* config,
    cfg::StreamType streamType);
void addOlympicAllSPQosMaps(cfg::SwitchConfig& cfg);

const std::map<int, std::vector<uint8_t>>& kOlympicAllSPQueueToDscp();
const std::vector<int>& kOlympicAllSPQueueIds();
cfg::ActiveQueueManagement kGetOlympicEcnConfig(
    int minLength = 41600,
    int maxLength = 41600);
cfg::ActiveQueueManagement kGetWredConfig(
    int minLength = 41600,
    int maxLength = 41600,
    int probability = 100);
void addQueueEcnConfig(
    cfg::SwitchConfig* config,
    const int queueId,
    const uint32_t minLen,
    const uint32_t maxLen);
void addQueueWredConfig(
    cfg::SwitchConfig* config,
    const int queueId,
    const uint32_t minLen,
    const uint32_t maxLen,
    const int probability);
void addQueueShaperConfig(
    cfg::SwitchConfig* config,
    const int queueId,
    const uint32_t minKbps,
    const uint32_t maxKbps);
void addQueueBurstSizeConfig(
    cfg::SwitchConfig* config,
    const int queueId,
    const uint32_t minKbits,
    const uint32_t maxKbits);

} // namespace facebook::fboss::utility
