/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/test/dataplane_tests/HwTestQueuePerHostUtils.h"

#include "fboss/agent/FbossError.h"
#include "fboss/agent/TxPacket.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwSwitchEnsemble.h"
#include "fboss/agent/hw/test/HwTestAclUtils.h"
#include "fboss/agent/hw/test/HwTestCoppUtils.h"
#include "fboss/agent/hw/test/dataplane_tests/HwTestUtils.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/utils/AclTestUtils.h"
#include "fboss/agent/test/utils/QueuePerHostTestUtils.h"
#include "fboss/agent/test/utils/TrafficPolicyTestUtils.h"

#include <folly/logging/xlog.h>

#include <string>

namespace facebook::fboss::utility {

namespace {

template <typename Hw>
void verifyQueuePerHostMappingImpl(
    Hw* hwOrEnsemble,
    AclStatGetFunc getAclStatFn,
    std::shared_ptr<SwitchState> swState,
    const std::vector<PortID>& portIds,
    std::optional<VlanID> vlanId,
    folly::MacAddress srcMac,
    folly::MacAddress dstMac,
    const folly::IPAddress& srcIp,
    const folly::IPAddress& dstIp,
    bool useFrontPanel,
    bool blockNeighbor,
    std::function<std::map<PortID, HwPortStats>(const std::vector<PortID>&)>
        getHwPortStatsFn,
    std::optional<uint16_t> l4SrcPort,
    std::optional<uint16_t> l4DstPort,
    std::optional<uint8_t> dscp) {
  HwSwitch* hwSwitch;
  if constexpr (std::is_same_v<Hw, HwSwitchEnsemble>) {
    hwSwitch = hwOrEnsemble->getHwSwitch();
  } else {
    hwSwitch = hwOrEnsemble;
  }
  auto ttlAclName = utility::getQueuePerHostTtlAclName();
  auto ttlCounterName = utility::getQueuePerHostTtlCounterName();

  auto statBefore = getAclStatFn(
      swState,
      ttlAclName,
      ttlCounterName,
      cfg::AclStage::INGRESS,
      std::nullopt);

  std::map<int, int64_t> beforeQueueOutPkts;
  for (const auto& queueId : utility::kQueuePerhostQueueIds()) {
    auto hwPortStatsMap = getHwPortStatsFn({portIds[0]});
    beforeQueueOutPkts[queueId] =
        hwPortStatsMap[portIds[0]].get_queueOutPackets_().at(queueId);
  }

  auto txPacket = utility::createUdpPkt(
      hwSwitch,
      vlanId,
      srcMac,
      dstMac,
      srcIp,
      dstIp,
      l4SrcPort.has_value() ? l4SrcPort.value() : 8000,
      l4DstPort.has_value() ? l4DstPort.value() : 8001,
      64 /* ttl < 128 */,
      dscp);
  auto txPacket2 = createUdpPkt(
      hwSwitch,
      vlanId,
      srcMac,
      dstMac,
      srcIp,
      dstIp,
      l4SrcPort.has_value() ? l4SrcPort.value() : 8000,
      l4DstPort.has_value() ? l4DstPort.value() : 8001,
      128 /* ttl >= 128 */,
      dscp);

  if (useFrontPanel) {
    utility::ensureSendPacketOutOfPort(
        hwOrEnsemble,
        std::move(txPacket),
        PortID(portIds[1]),
        portIds,
        getHwPortStatsFn);
    utility::ensureSendPacketOutOfPort(
        hwOrEnsemble,
        std::move(txPacket2),
        PortID(portIds[1]),
        portIds,
        getHwPortStatsFn);
  } else {
    utility::ensureSendPacketSwitched(
        hwOrEnsemble, std::move(txPacket), portIds, getHwPortStatsFn);
    utility::ensureSendPacketSwitched(
        hwOrEnsemble, std::move(txPacket2), portIds, getHwPortStatsFn);
  }

  WITH_RETRIES({
    std::map<int, int64_t> afterQueueOutPkts;
    for (const auto& queueId : utility::kQueuePerhostQueueIds()) {
      auto hwPortStatsMap = getHwPortStatsFn({portIds[0]});
      afterQueueOutPkts[queueId] =
          hwPortStatsMap[portIds[0]].get_queueOutPackets_().at(queueId);
    }

    /*
     *  Consider ACL with action to egress pkts through queue 2.
     *
     *  CPU originated packets:
     *     - Hits ACL (queue2Cnt = 1), egress through queue 2 of port0.
     *     - port0 is in loopback mode, so the packet gets looped back.
     *     - When packet is routed, its dstMAC gets overwritten. Thus, the
     *       looped back packet is not routed, and thus does not hit the ACL.
     *     - On some platforms, looped back packets for unknown MACs are
     *       flooded and counted on queue *before* the split horizon check
     *       (drop when srcPort == dstPort). This flooding always happens on
     *       queue 0, so expect one or more packets on queue 0.
     *
     *  Front panel packets (injected with pipeline bypass):
     *     - Egress out of port1 queue0 (pipeline bypass).
     *     - port1 is in loopback mode, so the packet gets looped back.
     *     - Rest of the workflow is same as above when CPU originated packet
     *       gets injected for switching.
     */
    for (auto [qid, beforePkts] : beforeQueueOutPkts) {
      auto pktsOnQueue = afterQueueOutPkts[qid] - beforePkts;

      XLOG(DBG2) << "queueId: " << qid << " pktsOnQueue: " << pktsOnQueue;

      if (blockNeighbor) {
        // if the neighbor is blocked, all pkts are dropped
        EXPECT_EVENTUALLY_EQ(pktsOnQueue, 0);
      } else {
        if (qid == kQueueId) {
          EXPECT_EVENTUALLY_EQ(pktsOnQueue, 2);
        } else if (qid == 0) {
          EXPECT_EVENTUALLY_GE(pktsOnQueue, 0);
        } else {
          EXPECT_EVENTUALLY_EQ(pktsOnQueue, 0);
        }
      }
    }

    auto aclStatsMatch = [&]() {
      auto statAfter = getAclStatFn(
          swState,
          ttlAclName,
          ttlCounterName,
          cfg::AclStage::INGRESS,
          std::nullopt);

      if (blockNeighbor) {
        // if the neighbor is blocked, all pkts are dropped
        return statAfter - statBefore == 0;
      }
      /*
       * counts ttl >= 128 packet only
       */
      return statAfter - statBefore == 1;
    };
    hwSwitch->updateStats();
    EXPECT_EVENTUALLY_TRUE(aclStatsMatch());
  });
}
} // namespace

void verifyQueuePerHostMapping(
    HwSwitchEnsemble* ensemble,
    std::shared_ptr<SwitchState> swState,
    const std::vector<PortID>& portIds,
    std::optional<VlanID> vlanId,
    folly::MacAddress srcMac,
    folly::MacAddress dstMac,
    const folly::IPAddress& srcIp,
    const folly::IPAddress& dstIp,
    bool useFrontPanel,
    bool blockNeighbor,
    std::function<std::map<PortID, HwPortStats>(const std::vector<PortID>&)>
        getHwPortStatsFn,
    std::optional<uint16_t> l4SrcPort,
    std::optional<uint16_t> l4DstPort,
    std::optional<uint8_t> dscp) {
  verifyQueuePerHostMappingImpl(
      ensemble,
      getAclStatGetFn(ensemble->getHwSwitch()),
      std::move(swState),
      portIds,
      vlanId,
      std::move(srcMac),
      std::move(dstMac),
      srcIp,
      dstIp,
      useFrontPanel,
      blockNeighbor,
      std::move(getHwPortStatsFn),
      l4SrcPort,
      l4DstPort,
      dscp);
}

void verifyQueuePerHostMapping(
    HwSwitch* hwSwitch,
    std::shared_ptr<SwitchState> swState,
    const std::vector<PortID>& portIds,
    std::optional<VlanID> vlanId,
    folly::MacAddress srcMac,
    folly::MacAddress dstMac,
    const folly::IPAddress& srcIp,
    const folly::IPAddress& dstIp,
    bool useFrontPanel,
    bool blockNeighbor,
    std::function<std::map<PortID, HwPortStats>(const std::vector<PortID>&)>
        getHwPortStatsFn,
    std::optional<uint16_t> l4SrcPort,
    std::optional<uint16_t> l4DstPort,
    std::optional<uint8_t> dscp) {
  verifyQueuePerHostMappingImpl(
      hwSwitch,
      getAclStatGetFn(hwSwitch),
      std::move(swState),
      portIds,
      vlanId,
      std::move(srcMac),
      std::move(dstMac),
      srcIp,
      dstIp,
      useFrontPanel,
      blockNeighbor,
      std::move(getHwPortStatsFn),
      l4SrcPort,
      l4DstPort,
      dscp);
}

void verifyQueuePerHostMapping(
    HwSwitchEnsemble* ensemble,
    std::optional<VlanID> vlanId,
    folly::MacAddress srcMac,
    folly::MacAddress dstMac,
    const folly::IPAddress& srcIp,
    const folly::IPAddress& dstIp,
    bool useFrontPanel,
    bool blockNeighbor,
    std::optional<uint16_t> l4SrcPort,
    std::optional<uint16_t> l4DstPort,
    std::optional<uint8_t> dscp) {
  // lambda that returns HwPortStats for the given port
  auto getPortStats =
      [&](const std::vector<PortID>& portIds) -> std::map<PortID, HwPortStats> {
    return ensemble->getLatestPortStats(portIds);
  };

  verifyQueuePerHostMapping(
      ensemble,
      ensemble->getProgrammedState(),
      ensemble->masterLogicalPortIds(),
      vlanId,
      srcMac,
      dstMac,
      srcIp,
      dstIp,
      useFrontPanel,
      blockNeighbor,
      getPortStats,
      l4SrcPort,
      l4DstPort,
      dscp);
}

} // namespace facebook::fboss::utility
