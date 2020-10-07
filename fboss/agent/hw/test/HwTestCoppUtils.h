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
#include "fboss/agent/hw/switch_asics/HwAsic.h"

#include <folly/IPAddress.h>
#include <string>

/*
 * This utility is to provide utils for bcm olympic tests.
 */

class HwSwitch;
namespace facebook::fboss::utility {

constexpr int kCPUPort = 0;

constexpr int kCoppLowPriQueueId = 0;
constexpr int kCoppDefaultPriQueueId = 1;
constexpr int kCoppMidPriQueueId = 2;

constexpr uint32_t kCoppLowPriWeight = 1;
constexpr uint32_t kCoppDefaultPriWeight = 1;
constexpr uint32_t kCoppMidPriWeight = 2;
constexpr uint32_t kCoppHighPriWeight = 4;

constexpr uint32_t kAveragePacketSize = 300;
constexpr uint32_t kCoppLowPriPktsPerSec = 100;
constexpr uint32_t kCoppDefaultPriPktsPerSec = 200;

constexpr uint16_t kBgpPort = 179;

// There should be no ACL/rxreasons matching this port
constexpr uint16_t kNonSpecialPort1 = 60000;
constexpr uint16_t kNonSpecialPort2 = 60001;

void addCpuQueueConfig(cfg::SwitchConfig& config, const HwAsic* hwAsic);

folly::CIDRNetwork kIPv6LinkLocalMcastNetwork();

folly::CIDRNetwork kIPv6LinkLocalUcastNetwork();

cfg::MatchAction createQueueMatchAction(int queueId);

std::vector<std::pair<cfg::AclEntry, cfg::MatchAction>> defaultCpuAcls(
    const HwAsic* hwAsic);

void addNoActionAclForNw(
    const folly::CIDRNetwork& nw,
    std::vector<std::pair<cfg::AclEntry, cfg::MatchAction>>& acls);

void addHighPriAclForNwAndNetworoControlDscp(
    const folly::CIDRNetwork& dstNetwork,
    int highPriQueueId,
    std::vector<std::pair<cfg::AclEntry, cfg::MatchAction>>& acls);

void addMidPriAclForNw(
    const folly::CIDRNetwork& dstNetwork,
    std::vector<std::pair<cfg::AclEntry, cfg::MatchAction>>& acls);

void setDefaultCpuTrafficPolicyConfig(
    cfg::SwitchConfig& config,
    const HwAsic* hwAsic);

cfg::Range getRange(uint32_t minimum, uint32_t maximum);

uint16_t getCoppHighPriQueueId(const HwAsic* hwAsic);

std::pair<uint64_t, uint64_t> getCpuQueueOutPacketsAndBytes(
    HwSwitch* hwSwitch,
    int queueId);
std::vector<cfg::PacketRxReasonToQueue> getCoppRxReasonToQueues(
    const HwAsic* hwAsic);

} // namespace facebook::fboss::utility
