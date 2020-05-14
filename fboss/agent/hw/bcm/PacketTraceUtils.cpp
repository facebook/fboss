// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/bcm/PacketTraceUtils.h"

#include <folly/logging/xlog.h>

#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmIntf.h"
#include "fboss/agent/hw/bcm/BcmPortTable.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/hw/bcm/gen-cpp2/packettrace_types.h"
#include "fboss/agent/state/Interface.h"

extern "C" {
#include <bcm/l3.h>
#include <bcm/port.h>
}

namespace {
/*
 * BroadView's hashInfo index needs to be amended with some special value
 * https://github.com/Broadcom-Switch/BroadView-Instrumentation/blob/master/
 * src/sb_plugin/sb_brcm/sb_common/packet_trace/
 * sbplugin_common_packet_trace.c#L29
 */
const int BVIEW_ECMP_IDX_START = 200000;
const int BVIEW_L3_EGRESS_IDX_START = 100000;

} // namespace

namespace facebook::fboss {

void transformPacketTraceInfo(
    const BcmSwitch* hwSwitch,
    const bcm_switch_pkt_trace_info_t& bcmPacketTraceInfo,
    PacketTraceInfo& packetTraceInfo) {
  const auto unit = hwSwitch->getUnit();

  // pkt_trace_lookup_status is a bitmap
  int length =
      sizeof(
          bcmPacketTraceInfo.pkt_trace_lookup_status.pkt_trace_status_bitmap) /
      sizeof(int32_t);
  const int32_t* bitmapPtr = reinterpret_cast<const int32_t*>(
      bcmPacketTraceInfo.pkt_trace_lookup_status.pkt_trace_status_bitmap);
  std::vector<int32_t> bitmap(bitmapPtr, bitmapPtr + length);
  *packetTraceInfo.lookupResult_ref() = std::move(bitmap);

  auto hashInfo = bcmPacketTraceInfo.pkt_trace_hash_info;

  if (hashInfo.flags & BCM_SWITCH_PKT_TRACE_ECMP_1) {
    int max_ecmp_size = 1;
    auto rv = bcm_l3_route_max_ecmp_get(unit, &max_ecmp_size);
    bcmCheckError(rv, "Could not get max ECMP size");

    std::vector<bcm_if_t> intfArray(max_ecmp_size);
    int multipathCount = 0;
    bcm_if_t ecmp_group_idx = hashInfo.ecmp_1_group > BVIEW_ECMP_IDX_START
        ? hashInfo.ecmp_1_group
        : hashInfo.ecmp_1_group + BVIEW_ECMP_IDX_START;
    rv = bcm_l3_egress_multipath_get(
        unit, ecmp_group_idx, max_ecmp_size, intfArray.data(), &multipathCount);
    bcmCheckError(rv, "Could not get array for ECMP group ", ecmp_group_idx);
    for (int i = 0; i < multipathCount; ++i) {
      bcm_l3_egress_t egressObject;
      rv = bcm_l3_egress_get(unit, intfArray[i], &egressObject);
      bcmCheckError(rv, "Could not get array for egress object ", intfArray[i]);
      packetTraceInfo.hashInfo_ref()->potentialEgressPorts_ref()->push_back(
          hwSwitch->getPortTable()->getBcmPort(egressObject.port)->getPortID());
    }

    bcm_l3_egress_t egressObject;
    bcm_if_t egress_idx = hashInfo.ecmp_1_egress > BVIEW_L3_EGRESS_IDX_START
        ? hashInfo.ecmp_1_egress
        : hashInfo.ecmp_1_egress + BVIEW_L3_EGRESS_IDX_START;
    rv = bcm_l3_egress_get(unit, egress_idx, &egressObject);
    bcmCheckError(rv, "Could not get array for egress object ", egress_idx);
    *packetTraceInfo.hashInfo_ref()->actualEgressPort_ref() =
        hwSwitch->getPortTable()->getBcmPort(egressObject.port)->getPortID();
  }

  // We haven't seen the following hashing in our switches.
  if (hashInfo.flags & BCM_SWITCH_PKT_TRACE_ECMP_2) {
    XLOG(INFO) << "Packet trace: ECMP 2 triggered.";
  }
  if (hashInfo.flags & BCM_SWITCH_PKT_TRACE_TRUNK) {
    XLOG(INFO) << "Packet trace: Trunk triggered.";
  }
  if (hashInfo.flags & BCM_SWITCH_PKT_TRACE_FABRIC_TRUNK) {
    XLOG(INFO) << "Packet trace: Fabric trunk triggered.";
  }

  *packetTraceInfo.resolution_ref() = bcmPacketTraceInfo.pkt_trace_resolution;
  *packetTraceInfo.stpState_ref() = bcmPacketTraceInfo.pkt_trace_stp_state;
  *packetTraceInfo.destPipeNum_ref() = bcmPacketTraceInfo.dest_pipe_num;
}

} // namespace facebook::fboss
