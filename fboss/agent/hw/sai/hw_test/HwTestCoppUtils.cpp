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

HwPortStats getCpuQueueStats(HwSwitch* hwSwitch) {
  auto saiSwitch = static_cast<SaiSwitch*>(hwSwitch);
  saiSwitch->updateStats();
  return saiSwitch->managerTable()->hostifManager().getCpuPortStats();
}

HwPortStats getCpuQueueWatermarkStats(HwSwitch* hwSwitch) {
  // CPU Queue stats include watermark stats as well
  return getCpuQueueStats(hwSwitch);
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
          cfg::PacketRxReason::NDP, coppHighPriQueueId),
      ControlPlane::makeRxReasonToQueueEntry(
          cfg::PacketRxReason::BGP, coppHighPriQueueId),
      ControlPlane::makeRxReasonToQueueEntry(
          cfg::PacketRxReason::BGPV6, coppHighPriQueueId),
      ControlPlane::makeRxReasonToQueueEntry(
          cfg::PacketRxReason::CPU_IS_NHOP, kCoppMidPriQueueId),
      ControlPlane::makeRxReasonToQueueEntry(
          cfg::PacketRxReason::LACP, coppHighPriQueueId),
      ControlPlane::makeRxReasonToQueueEntry(
          cfg::PacketRxReason::TTL_1, kCoppLowPriQueueId),
      ControlPlane::makeRxReasonToQueueEntry(
          cfg::PacketRxReason::LLDP, kCoppMidPriQueueId),
      ControlPlane::makeRxReasonToQueueEntry(
          cfg::PacketRxReason::DHCP, kCoppMidPriQueueId),
      ControlPlane::makeRxReasonToQueueEntry(
          cfg::PacketRxReason::DHCPV6, kCoppMidPriQueueId),
  };

  // TODO(daiweix): remove after L4 port match is supported by J3 in 6.5.30
  if (hwAsic->getAsicType() == cfg::AsicType::ASIC_TYPE_JERICHO3) {
    rxReasonToQueues = {
        ControlPlane::makeRxReasonToQueueEntry(
            cfg::PacketRxReason::ARP, coppHighPriQueueId),
        ControlPlane::makeRxReasonToQueueEntry(
            cfg::PacketRxReason::ARP_RESPONSE, coppHighPriQueueId),
        ControlPlane::makeRxReasonToQueueEntry(
            cfg::PacketRxReason::NDP, coppHighPriQueueId),
        ControlPlane::makeRxReasonToQueueEntry(
            cfg::PacketRxReason::BGP, coppHighPriQueueId),
        ControlPlane::makeRxReasonToQueueEntry(
            cfg::PacketRxReason::CPU_IS_NHOP, kCoppMidPriQueueId),
        ControlPlane::makeRxReasonToQueueEntry(
            cfg::PacketRxReason::LACP, coppHighPriQueueId),
        ControlPlane::makeRxReasonToQueueEntry(
            cfg::PacketRxReason::TTL_1, kCoppLowPriQueueId),
        ControlPlane::makeRxReasonToQueueEntry(
            cfg::PacketRxReason::LLDP, kCoppMidPriQueueId),
        ControlPlane::makeRxReasonToQueueEntry(
            cfg::PacketRxReason::DHCP, kCoppMidPriQueueId),
    };
  }

  if (hwAsic->isSupported(HwAsic::Feature::SAI_EAPOL_TRAP)) {
    rxReasonToQueues.push_back(ControlPlane::makeRxReasonToQueueEntry(
        cfg::PacketRxReason::EAPOL, coppHighPriQueueId));
  }

  if (hwAsic->isSupported(HwAsic::Feature::SAI_MPLS_TTL_1_TRAP)) {
#if !defined(TAJO_SDK_VERSION_1_42_8)
    rxReasonToQueues.push_back(ControlPlane::makeRxReasonToQueueEntry(
        cfg::PacketRxReason::MPLS_TTL_1, kCoppLowPriQueueId));
#endif
  }

  if (hwAsic->isSupported(HwAsic::Feature::SAI_SAMPLEPACKET_TRAP)) {
    rxReasonToQueues.push_back(ControlPlane::makeRxReasonToQueueEntry(
        cfg::PacketRxReason::SAMPLEPACKET, kCoppLowPriQueueId));
  }

  // TODO: remove once CS00012311423 is fixed. Gate setting the L3 mtu error
  // trap on J2/J3 more specifically.
  if (hwAsic->isSupported(HwAsic::Feature::L3_MTU_ERROR_TRAP)) {
    rxReasonToQueues.push_back(ControlPlane::makeRxReasonToQueueEntry(
        cfg::PacketRxReason::L3_MTU_ERROR, kCoppLowPriQueueId));
  }

  return rxReasonToQueues;
}

} // namespace utility
} // namespace facebook::fboss
