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

#include <folly/IPAddress.h>
#include <string>

/*
 * This utility is to provide utils for bcm olympic tests.
 */
namespace facebook {
namespace fboss {
class HwSwitch;
namespace utility {

constexpr int kCPUPort = 0;

constexpr int kCoppLowPriQueueId = 0;
constexpr int kCoppDefaultPriQueueId = 1;
constexpr int kCoppMidPriQueueId = 2;
constexpr int kCoppHighPriQueueId = 9;

constexpr uint32_t kCoppLowPriWeight = 1;
constexpr uint32_t kCoppDefaultPriWeight = 1;
constexpr uint32_t kCoppMidPriWeight = 2;
constexpr uint32_t kCoppHighPriWeight = 4;

constexpr uint32_t kCoppLowPriPktsPerSec = 100;
constexpr uint32_t kCoppDefaultPriPktsPerSec = 200;

constexpr uint16_t kBgpPort = 179;

// There should be no ACL/rxreasons matching this port
constexpr uint16_t kNonSpecialPort1 = 60000;
constexpr uint16_t kNonSpecialPort2 = 60001;

void addCpuQueueConfig(cfg::SwitchConfig& config);

std::vector<std::pair<cfg::AclEntry, cfg::MatchAction>> defaultCpuAcls(
    const folly::MacAddress& localMac);
void setDefaultCpuTrafficPolicyConfig(
    cfg::SwitchConfig& config,
    const folly::MacAddress& localMac);

cfg::Range getRange(uint32_t minimum, uint32_t maximum);

uint64_t getCpuQueueOutPackets(HwSwitch* hwSwitch, int queueId);

} // namespace utility
} // namespace fboss
} // namespace facebook
