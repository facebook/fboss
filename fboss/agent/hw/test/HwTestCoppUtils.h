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
#include "fboss/agent/HwSwitch.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/gen-cpp2/hardware_stats_types.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/types.h"

#include <folly/IPAddress.h>
#include <folly/logging/xlog.h>
#include <gtest/gtest.h>
#include <string>

/*
 * This utility is to provide utils for bcm olympic tests.
 */

namespace facebook::fboss {
class HwPortStats;
class HwSwitch;
class SwitchState;

namespace utility {

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

// Tajo supports higher PPS to CPU
constexpr uint32_t kCpuPacketOverheadBytes = 52;
constexpr uint32_t kCoppTajoLowPriPktsPerSec = 10000;
constexpr uint32_t kCoppTajoDefaultPriPktsPerSec = 20000;

constexpr uint16_t kBgpPort = 179;

// There should be no ACL/rxreasons matching this port
constexpr uint16_t kNonSpecialPort1 = 60000;
constexpr uint16_t kNonSpecialPort2 = 60001;

// For benchmark tests, we don't want to set queue rate for low priority queues.
void addCpuQueueConfig(
    cfg::SwitchConfig& config,
    const HwAsic* hwAsic,
    bool setQueueRate = true);

folly::CIDRNetwork kIPv6LinkLocalMcastNetwork();

folly::CIDRNetwork kIPv6LinkLocalUcastNetwork();

folly::CIDRNetwork kIPv6NdpSolicitNetwork();

cfg::MatchAction createQueueMatchAction(
    int queueId,
    cfg::ToCpuAction toCpuAction);

std::vector<std::pair<cfg::AclEntry, cfg::MatchAction>> defaultCpuAcls(
    const HwAsic* hwAsic,
    cfg::SwitchConfig& config);

std::string getMplsDestNoMatchCounterName(void);

void addNoActionAclForNw(
    const folly::CIDRNetwork& nw,
    std::vector<std::pair<cfg::AclEntry, cfg::MatchAction>>& acls);

void addHighPriAclForNwAndNetworkControlDscp(
    const folly::CIDRNetwork& dstNetwork,
    int highPriQueueId,
    cfg::ToCpuAction toCpuAction,
    std::vector<std::pair<cfg::AclEntry, cfg::MatchAction>>& acls);

void addMidPriAclForNw(
    const folly::CIDRNetwork& dstNetwork,
    cfg::ToCpuAction toCpuAction,
    std::vector<std::pair<cfg::AclEntry, cfg::MatchAction>>& acls);

void setDefaultCpuTrafficPolicyConfig(
    cfg::SwitchConfig& config,
    const HwAsic* hwAsic);

cfg::StreamType getCpuDefaultStreamType(const HwAsic* hwAsic);

cfg::Range getRange(uint32_t minimum, uint32_t maximum);

uint16_t getCoppHighPriQueueId(const HwAsic* hwAsic);

uint32_t getCoppQueuePps(const HwAsic* hwAsic, uint16_t queueId);

cfg::ToCpuAction getCpuActionType(const HwAsic* hwAsic);

std::pair<uint64_t, uint64_t> getCpuQueueOutPacketsAndBytes(
    HwSwitch* hwSwitch,
    int queueId);
std::pair<uint64_t, uint64_t> getCpuQueueOutDiscardPacketsAndBytes(
    HwSwitch* hwSwitch,
    int queueId);
uint64_t getCpuQueueWatermarkBytes(HwPortStats& stats, int queueId);
HwPortStats getCpuQueueStats(HwSwitch* hwSwitch);
HwPortStats getCpuQueueWatermarkStats(HwSwitch* hwSwitch);
std::vector<cfg::PacketRxReasonToQueue> getCoppRxReasonToQueues(
    const HwAsic* hwAsic);

void setPortQueueSharedBytes(cfg::PortQueue& queue);

std::unique_ptr<facebook::fboss::TxPacket> createUdpPkt(
    const HwSwitch* hwSwitch,
    std::optional<VlanID> vlanId,
    folly::MacAddress srcMac,
    folly::MacAddress dstMac,
    const folly::IPAddress& srcIpAddress,
    const folly::IPAddress& dstIpAddress,
    int l4SrcPort,
    int l4DstPort,
    uint8_t ttl,
    std::optional<uint8_t> dscp);

uint64_t getQueueOutPacketsWithRetry(
    HwSwitch* hwSwitch,
    int queueId,
    int retryTimes,
    uint64_t expectedNumPkts,
    int postMatchRetryTimes = 2);

template <typename SendFn>
void sendPktAndVerifyCpuQueue(
    HwSwitch* hwSwitch,
    int queueId,
    SendFn sendPkts,
    const int expectedPktDelta) {
  auto beforeOutPkts = getQueueOutPacketsWithRetry(
      hwSwitch, queueId, 0 /* retryTimes */, 0 /* expectedNumPkts */);
  sendPkts();
  constexpr auto kGetQueueOutPktsRetryTimes = 5;
  auto afterOutPkts = getQueueOutPacketsWithRetry(
      hwSwitch,
      queueId,
      kGetQueueOutPktsRetryTimes,
      beforeOutPkts + expectedPktDelta);
  XLOG(DBG0) << "Queue=" << queueId << ", before pkts:" << beforeOutPkts
             << ", after pkts:" << afterOutPkts;
  EXPECT_EQ(expectedPktDelta, afterOutPkts - beforeOutPkts);
}

void sendAndVerifyPkts(
    HwSwitch* hwSwitch,
    std::shared_ptr<SwitchState> swState,
    const folly::IPAddress& destIp,
    uint16_t destPort,
    uint8_t queueId,
    PortID srcPort);

void verifyCoppInvariantHelper(
    HwSwitch* hwSwitch,
    const HwAsic* hwAsic,
    std::shared_ptr<SwitchState> swState,
    PortID srcPort);

} // namespace utility
} // namespace facebook::fboss
