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

#include "fboss/agent/SwitchStats.h"

namespace facebook {
namespace fboss {
namespace utility {

uint64_t getCpuQueueOutPackets(HwSwitch* hwSwitch, int queueId) {
  auto saiSwitch = static_cast<SaiSwitch*>(hwSwitch);
  SwitchStats dummy;
  saiSwitch->updateStats(&dummy);
  auto hwPortStats =
      saiSwitch->managerTable()->hostifManager().getCpuPortStats();
  auto queueIter = hwPortStats.queueOutPackets_.find(queueId);
  return (queueIter != hwPortStats.queueOutPackets_.end()) ? queueIter->second
                                                           : 0;
}

std::map<cfg::PacketRxReason, int16_t> getCoppRxReasonToQueues() {
  return std::map<cfg::PacketRxReason, int16_t>{
      {cfg::PacketRxReason::BGP, kCoppHighPriQueueId},
      {cfg::PacketRxReason::BGPV6, kCoppHighPriQueueId},
      {cfg::PacketRxReason::ARP, kCoppHighPriQueueId},
      {cfg::PacketRxReason::ARP_RESPONSE, kCoppHighPriQueueId},
      {cfg::PacketRxReason::CPU_IS_NHOP, kCoppLowPriQueueId}};
}
} // namespace utility
} // namespace fboss
} // namespace facebook
