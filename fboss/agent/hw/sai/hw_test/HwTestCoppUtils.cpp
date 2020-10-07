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

#include "fboss/agent/hw/sai/switch/SaiHostifManager.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiSwitch.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"

#include "fboss/agent/SwitchStats.h"
#include "fboss/agent/state/ControlPlane.h"

namespace facebook::fboss {

namespace utility {

std::pair<uint64_t, uint64_t> getCpuQueueOutPacketsAndBytes(
    HwSwitch* hwSwitch,
    int queueId) {
  auto saiSwitch = static_cast<SaiSwitch*>(hwSwitch);
  SwitchStats dummy;
  saiSwitch->updateStats(&dummy);
  auto hwPortStats =
      saiSwitch->managerTable()->hostifManager().getCpuPortStats();
  auto queueIter = hwPortStats.queueOutPackets__ref()->find(queueId);
  auto outPackets = (queueIter != hwPortStats.queueOutPackets__ref()->end())
      ? queueIter->second
      : 0;
  queueIter = hwPortStats.queueOutBytes__ref()->find(queueId);
  auto outBytes = (queueIter != hwPortStats.queueOutPackets__ref()->end())
      ? queueIter->second
      : 0;
  return std::pair(outPackets, outBytes);
}

std::vector<std::pair<cfg::AclEntry, cfg::MatchAction>> defaultCpuAcls(
    const HwAsic* hwAsic) {
  std::vector<std::pair<cfg::AclEntry, cfg::MatchAction>> acls;

  // multicast link local dst ip
  addNoActionAclForNw(kIPv6LinkLocalMcastNetwork(), acls);

  // Link local IPv6 + DSCP 48 to high pri queue
  addHighPriAclForNwAndNetworoControlDscp(
      kIPv6LinkLocalMcastNetwork(), getCoppHighPriQueueId(hwAsic), acls);
  addHighPriAclForNwAndNetworoControlDscp(
      kIPv6LinkLocalUcastNetwork(), getCoppHighPriQueueId(hwAsic), acls);

  // unicast and multicast link local dst ip
  addMidPriAclForNw(kIPv6LinkLocalMcastNetwork(), acls);
  // All fe80::/10 to mid pri queue
  addMidPriAclForNw(kIPv6LinkLocalUcastNetwork(), acls);

  return acls;
}
std::vector<cfg::PacketRxReasonToQueue> getCoppRxReasonToQueues(
    const HwAsic* hwAsic) {
  auto coppHighPriQueueId = utility::getCoppHighPriQueueId(hwAsic);
  ControlPlane::RxReasonToQueue rxReasonToQueues = {
      ControlPlane::makeRxReasonToQueueEntry(
          cfg::PacketRxReason::ARP, coppHighPriQueueId),
      ControlPlane::makeRxReasonToQueueEntry(
          cfg::PacketRxReason::ARP_RESPONSE, coppHighPriQueueId),
      ControlPlane::makeRxReasonToQueueEntry(
          cfg::PacketRxReason::BGP, coppHighPriQueueId),
      ControlPlane::makeRxReasonToQueueEntry(
          cfg::PacketRxReason::BGPV6, coppHighPriQueueId),
      ControlPlane::makeRxReasonToQueueEntry(
          cfg::PacketRxReason::CPU_IS_NHOP, kCoppMidPriQueueId),
      ControlPlane::makeRxReasonToQueueEntry(
          cfg::PacketRxReason::L3_MTU_ERROR, kCoppLowPriQueueId),
      ControlPlane::makeRxReasonToQueueEntry(
          cfg::PacketRxReason::LACP, coppHighPriQueueId),
      ControlPlane::makeRxReasonToQueueEntry(
          cfg::PacketRxReason::TTL_1, kCoppLowPriQueueId),
  };
  return rxReasonToQueues;
}

} // namespace utility
} // namespace facebook::fboss
