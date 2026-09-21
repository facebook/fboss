// Copyright 2004-present Facebook. All Rights Reserved.
#pragma once

extern "C" {
#include <bcm/switch.h>
}

namespace facebook::fboss {

class BcmSwitch;
class PacketTraceInfo;

void transformPacketTraceInfo(
    const BcmSwitch* hwSwitch,
    const bcm_switch_pkt_trace_info_t& bcmPacketTraceInfo,
    PacketTraceInfo& packetTraceInfo);

} // namespace facebook::fboss
