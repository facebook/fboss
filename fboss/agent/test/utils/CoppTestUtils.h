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
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/gen-cpp2/hardware_stats_types.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/test/utils/AsicUtils.h"
#include "fboss/agent/types.h"

#include <folly/IPAddress.h>
#include <folly/logging/xlog.h>
#include <gtest/gtest.h>
#include <cstdint>
#include <string>

/*
 * This utility is to provide utils for bcm olympic tests.
 */

namespace facebook::fboss {
class HwPortStats;
class SwitchState;
class HwSwitch;
class TestEnsembleIf;

namespace utility {

constexpr int kCPUPort = 0;

constexpr int kCoppLowPriQueueId = 0;
constexpr int kCoppDefaultPriQueueId = 1;
constexpr int kCoppMidPriQueueId = 2;
// avoid using cpu queue 2 on J3 platforms, because this queue 2 is
// currently set lossless across all port including cpu port.
// Since cpu port is slow, sending traffic to this queue 2 could cause
// head of line block issue like T182789218.
constexpr int kJ3CoppMidPriQueueId = 3;

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

// DNX supports higher PPS to CPU
constexpr uint32_t kCoppDnxLowPriPktsPerSec = 12000;
constexpr uint32_t kCoppDnxDefaultPriPktsPerSec = 24000;
constexpr uint32_t kCoppDnxLowPriKbitsPerSec = 100 * 1024;

constexpr uint16_t kBgpPort = 179;

// There should be no ACL/rxreasons matching this port
constexpr uint16_t kNonSpecialPort1 = 60000;
constexpr uint16_t kNonSpecialPort2 = 60001;

// For benchmark tests, we don't want to set queue rate for low priority queues.
void addCpuQueueConfig(
    cfg::SwitchConfig& config,
    const std::vector<const HwAsic*>& asics,
    bool isSai,
    bool setQueueRate = true);

folly::CIDRNetwork kIPv6LinkLocalMcastNetwork();

folly::CIDRNetwork kIPv6LinkLocalUcastNetwork();

folly::CIDRNetwork kIPv6NdpSolicitNetwork();

cfg::MatchAction
createQueueMatchAction(int queueId, bool isSai, cfg::ToCpuAction toCpuAction);

std::vector<std::pair<cfg::AclEntry, cfg::MatchAction>>
defaultCpuAcls(const HwAsic* hwAsic, cfg::SwitchConfig& config, bool isSai);

uint16_t getNumDefaultCpuAcls(const HwAsic* hwAsic, bool isSai);

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

void addLowPriAclForUnresolvedRoutes(
    cfg::ToCpuAction toCpuAction,
    std::vector<std::pair<cfg::AclEntry, cfg::MatchAction>>& acls);

void addLowPriAclForConnectedSubnetRoutes(
    cfg::ToCpuAction toCpuAction,
    std::vector<std::pair<cfg::AclEntry, cfg::MatchAction>>& acls);

void setDefaultCpuTrafficPolicyConfig(
    cfg::SwitchConfig& config,
    const std::vector<const HwAsic*>& asics,
    bool isSai);

cfg::StreamType getCpuDefaultStreamType(const HwAsic* hwAsic);
cfg::QueueScheduling getCpuDefaultQueueScheduling(const HwAsic* hwAsic);

cfg::Range getRange(uint32_t minimum, uint32_t maximum);

uint16_t getCoppHighPriQueueId(const HwAsic* hwAsic);

uint16_t getCoppHighPriQueueId(const std::vector<const HwAsic*>& hwAsics);
uint16_t getCoppMidPriQueueId(const std::vector<const HwAsic*>& hwAsics);

std::shared_ptr<facebook::fboss::Interface> getEligibleInterface(
    std::shared_ptr<SwitchState> swState);

uint32_t getCoppQueuePps(const HwAsic* hwAsic, uint16_t queueId);

cfg::ToCpuAction getCpuActionType(const HwAsic* hwAsic);

uint64_t getCpuQueueWatermarkBytes(HwPortStats& stats, int queueId);
std::vector<cfg::PacketRxReasonToQueue> getCoppRxReasonToQueues(
    const std::vector<const HwAsic*>& hwAsics,
    bool isSai);

std::pair<uint64_t, uint64_t> getCpuQueueOutPacketsAndBytes(
    HwPortStats& stats,
    int queueId);

std::pair<uint64_t, uint64_t>
getCpuQueueOutPacketsAndBytes(SwSwitch* sw, int queueId, SwitchID switchId);

std::pair<uint64_t, uint64_t> getCpuQueueOutPacketsAndBytes(
    HwSwitch* hw,
    int queueId,
    SwitchID switchId = SwitchID(0));

void setPortQueueSharedBytes(cfg::PortQueue& queue, bool isSai);

void setPortQueueMaxDynamicSharedBytes(
    cfg::PortQueue& queue,
    const HwAsic* hwAsic);

void setTTLZeroCpuConfig(
    const std::vector<const HwAsic*>& asics,
    cfg::SwitchConfig& config);

void addTrafficCounter(
    cfg::SwitchConfig* config,
    const std::string& counterName,
    std::optional<std::vector<cfg::CounterType>> counterTypes);

cfg::MatchAction getToQueueAction(
    const int queueId,
    bool isSai,
    const std::optional<cfg::ToCpuAction> toCpuAction = std::nullopt);

void addNoActionAclForUnicastLinkLocal(
    const folly::CIDRNetwork& nw,
    std::vector<std::pair<cfg::AclEntry, cfg::MatchAction>>& acls);

template <typename SwitchT>
uint64_t getQueueOutPacketsWithRetry(
    SwitchT* switchPtr,
    SwitchID switchId,
    int queueId,
    int retryTimes,
    uint64_t expectedNumPkts,
    int postMatchRetryTimes = 2);

template <typename SendFn, typename SwitchT>
void sendPktAndVerifyCpuQueue(
    SwitchT* switchPtr,
    SwitchID switchId,
    int queueId,
    SendFn sendPkts,
    const int expectedPktDelta) {
  auto beforeOutPkts = getQueueOutPacketsWithRetry(
      switchPtr,
      switchId,
      queueId,
      0 /* retryTimes */,
      0 /* expectedNumPkts */,
      2 /* postMatchRetryTimes */);
  sendPkts();
  constexpr auto kGetQueueOutPktsRetryTimes = 5;
  auto afterOutPkts = getQueueOutPacketsWithRetry(
      switchPtr,
      switchId,
      queueId,
      kGetQueueOutPktsRetryTimes,
      beforeOutPkts + expectedPktDelta);
  XLOG(DBG0) << "Queue=" << queueId << ", before pkts:" << beforeOutPkts
             << ", after pkts:" << afterOutPkts;
  EXPECT_EQ(expectedPktDelta, afterOutPkts - beforeOutPkts);
}

uint64_t getCpuQueueInPackets(SwSwitch* sw, SwitchID switchId, int queueId);
CpuPortStats getLatestCpuStats(SwSwitch* sw, SwitchID switchId);

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

std::unique_ptr<facebook::fboss::TxPacket> createUdpPkt(
    TestEnsembleIf* ensemble,
    std::optional<VlanID> vlanId,
    folly::MacAddress srcMac,
    folly::MacAddress dstMac,
    const folly::IPAddress& srcIpAddress,
    const folly::IPAddress& dstIpAddress,
    int l4SrcPort,
    int l4DstPort,
    uint8_t ttl,
    std::optional<uint8_t> dscp);

template <typename SwitchT>
void sendAndVerifyPkts(
    SwitchT* switchPtr,
    SwitchID switchId,
    std::shared_ptr<SwitchState> swState,
    const folly::IPAddress& destIp,
    uint16_t destPort,
    uint8_t queueId,
    PortID srcPort,
    uint8_t trafficClass = 0);

template <typename SwitchT>
void verifyCoppInvariantHelper(
    SwitchT* switchPtr,
    SwitchID switchId,
    const HwAsic* hwAsic,
    std::shared_ptr<SwitchState> swState,
    PortID srcPort);

void excludeTTL1TrapConfig(cfg::SwitchConfig& config);

CpuPortStats getCpuPortStats(SwSwitch* sw, SwitchID switchId);

cfg::PortQueueRate setPortQueueRate(const HwAsic* hwAsic, uint16_t queueId);

uint32_t getDnxCoppMaxDynamicSharedBytes(uint16_t queueId);

AgentConfig setTTL0PacketForwardingEnableConfig(
    SwSwitch* sw,
    AgentConfig& agentConfig);

} // namespace utility
} // namespace facebook::fboss
