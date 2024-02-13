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
#include "fboss/agent/test/utils/CoppTestUtils.h"
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
class HwSwitch;
class SwitchState;

namespace utility {

std::pair<uint64_t, uint64_t> getCpuQueueOutPacketsAndBytes(
    HwSwitch* hwSwitch,
    int queueId);
std::pair<uint64_t, uint64_t> getCpuQueueOutDiscardPacketsAndBytes(
    HwSwitch* hwSwitch,
    int queueId);
HwPortStats getCpuQueueStats(HwSwitch* hwSwitch);
HwPortStats getCpuQueueWatermarkStats(HwSwitch* hwSwitch);

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
