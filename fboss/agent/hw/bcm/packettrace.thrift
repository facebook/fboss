namespace cpp2 facebook.fboss
namespace d neteng.fboss.packettrace
namespace py neteng.fboss.packettrace

/*
 * This is modelled after Broadcom BroadView Packet Trace (PT)
 * https://github.com/Broadcom-Switch/OpenNSL/blob/master/include/opennsl/switch.h
 */

struct PacketTraceHashingInfo {
  1: list<i32> potentialEgressPorts, // All PortIDs for the ECMP group
  2: i32 actualEgressPort, // PortIDs for the actual egress
}

struct PacketTraceInfo {
  1: list<i32> lookupResult,
  2: i32 resolution = -1,
  3: PacketTraceHashingInfo hashInfo,
  4: i32 stpState = -1,
  5: i32 destPipeNum = -1,
}
