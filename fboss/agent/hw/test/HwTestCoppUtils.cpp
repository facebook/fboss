/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/test/HwTestCoppUtils.h"
#include <vector>

#include "fboss/agent/FbossError.h"
#include "fboss/agent/HwSwitch.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/hw/test/HwTestAclUtils.h"
#include "fboss/agent/hw/test/HwTestPacketUtils.h"
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/ResourceLibUtil.h"

#include "fboss/agent/test/utils/CoppTestUtils.h"

DECLARE_bool(enable_acl_table_group);

namespace facebook::fboss::utility {

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
    std::optional<uint8_t> dscp) {
  auto txPacket = utility::makeUDPTxPacket(
      hwSwitch,
      vlanId,
      srcMac,
      dstMac,
      srcIpAddress,
      dstIpAddress,
      l4SrcPort,
      l4DstPort,
      (dscp.has_value() ? dscp.value() : 48) << 2,
      ttl);

  return txPacket;
}

uint64_t getQueueOutPacketsWithRetry(
    HwSwitch* hwSwitch,
    int queueId,
    int retryTimes,
    uint64_t expectedNumPkts,
    int postMatchRetryTimes) {
  uint64_t outPkts = 0, outBytes = 0;
  do {
    for (auto i = 0; i <=
         utility::getCoppHighPriQueueId(hwSwitch->getPlatform()->getAsic());
         i++) {
      auto [qOutPkts, qOutBytes] =
          utility::getCpuQueueOutPacketsAndBytes(hwSwitch, i);
      XLOG(DBG2) << "QueueID: " << i << " qOutPkts: " << qOutPkts
                 << " outBytes: " << qOutBytes;
    }
    std::tie(outPkts, outBytes) =
        getCpuQueueOutPacketsAndBytes(hwSwitch, queueId);
    if (retryTimes == 0 || (outPkts >= expectedNumPkts)) {
      break;
    }

    /*
     * Post warmboot, the packet always gets processed by the right CPU
     * queue (as per ACL/rxreason etc.) but sometimes it is delayed.
     * Retrying a few times to avoid test noise.
     */
    XLOG(DBG0) << "Retry...";
    /* sleep override */
    sleep(1);
  } while (retryTimes-- > 0);

  while ((outPkts == expectedNumPkts) && postMatchRetryTimes--) {
    std::tie(outPkts, outBytes) =
        getCpuQueueOutPacketsAndBytes(hwSwitch, queueId);
  }

  return outPkts;
}

std::pair<uint64_t, uint64_t> getCpuQueueOutPacketsAndBytes(
    HwSwitch* hwSwitch,
    int queueId) {
  auto hwPortStats = getCpuQueueStats(hwSwitch);
  auto queueIter = hwPortStats.queueOutPackets_()->find(queueId);
  auto outPackets = (queueIter != hwPortStats.queueOutPackets_()->end())
      ? queueIter->second
      : 0;
  queueIter = hwPortStats.queueOutBytes_()->find(queueId);
  auto outBytes = (queueIter != hwPortStats.queueOutBytes_()->end())
      ? queueIter->second
      : 0;
  return std::pair(outPackets, outBytes);
}

std::pair<uint64_t, uint64_t> getCpuQueueOutDiscardPacketsAndBytes(
    HwSwitch* hwSwitch,
    int queueId) {
  auto hwPortStats = getCpuQueueStats(hwSwitch);
  auto queueIter = hwPortStats.queueOutDiscardPackets_()->find(queueId);
  auto outDiscardPackets =
      (queueIter != hwPortStats.queueOutDiscardPackets_()->end())
      ? queueIter->second
      : 0;
  queueIter = hwPortStats.queueOutDiscardBytes_()->find(queueId);
  auto outDiscardBytes =
      (queueIter != hwPortStats.queueOutDiscardBytes_()->end())
      ? queueIter->second
      : 0;
  return std::pair(outDiscardPackets, outDiscardBytes);
}

void sendAndVerifyPkts(
    HwSwitch* hwSwitch,
    std::shared_ptr<SwitchState> swState,
    const folly::IPAddress& destIp,
    uint16_t destPort,
    uint8_t queueId,
    PortID srcPort) {
  auto sendPkts = [&] {
    auto vlanId = utility::firstVlanID(swState);
    auto intfMac = utility::getFirstInterfaceMac(swState);
    utility::sendTcpPkts(
        hwSwitch,
        1 /*numPktsToSend*/,
        vlanId,
        intfMac,
        destIp,
        utility::kNonSpecialPort1,
        destPort,
        srcPort);
  };

  sendPktAndVerifyCpuQueue(hwSwitch, queueId, sendPkts, 1);
}

void verifyCoppInvariantHelper(
    HwSwitch* hwSwitch,
    const HwAsic* hwAsic,
    std::shared_ptr<SwitchState> swState,
    PortID srcPort) {
  auto intf = getEligibleInterface(swState);
  if (!intf) {
    throw FbossError(
        "No eligible uplink/downlink interfaces in config to verify COPP invariant");
  }
  for (auto iter : std::as_const(*intf->getAddresses())) {
    auto destIp = folly::IPAddress(iter.first);
    if (destIp.isLinkLocal()) {
      // three elements in the address vector: ipv4, ipv6 and a link local one
      // if the address qualifies as link local, it will loop back to the queue
      // again, adding an extra packet to the queue and failing the verification
      // thus, we skip the last one and only send BGP packets to v4 and v6 addr
      continue;
    }
    sendAndVerifyPkts(
        hwSwitch,
        swState,
        destIp,
        utility::kBgpPort,
        utility::getCoppHighPriQueueId(hwAsic),
        srcPort);
  }
  auto addrs = intf->getAddressesCopy();
  sendAndVerifyPkts(
      hwSwitch,
      swState,
      addrs.begin()->first,
      utility::kNonSpecialPort2,
      utility::kCoppMidPriQueueId,
      srcPort);
}

} // namespace facebook::fboss::utility
