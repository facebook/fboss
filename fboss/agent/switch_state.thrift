#
# Copyright 2004-present Facebook. All Rights Reserved.
#
namespace py neteng.fboss.switch_state
namespace py3 neteng.fboss
namespace py.asyncio neteng.fboss.asyncio.switch_state
namespace cpp2 facebook.fboss.state

include "fboss/agent/switch_config.thrift"

struct VlanInfo {
 1: bool tagged;
}

// Port queueing configuration
struct PortQueueFields {
 1: i16 id
 2: i32 weight
 3: optional i32 reserved
 // TODO: replace with switch_config.MMUScalingFactor?
 4: optional string scalingFactor
 // TODO: replace with switch_config.QueueScheduling?
 5: string scheduling
 // TODO: replace with switch_config.StreamType?
 6: string streamType
 7: optional list<switch_config.ActiveQueueManagement> aqms
 8: optional string name
 /*
  * Refer PortQueueRate which is a generalized version and allows configuring
  * pps as well as kbps.
  */
 10: optional i32 packetsPerSec_DEPRECATED
 11: optional i32 sharedBytes
 12: optional switch_config.PortQueueRate portQueueRate

 13: optional i32 bandwidthBurstMinKbits
 14: optional i32 bandwidthBurstMaxKbits
 15: optional i16 trafficClass
}

// Port configuration and oper state fields
// TODO: separate config and operational state
struct PortFields {
 1: required i32 portId
 2: required string portName
 3: string portDescription
 // TODO: use switch_config.PortState?
 4: string portState
 5: bool portOperState
 6: i32 ingressVlan
 // TODO: use switch_config.PortSpeed?
 7: string portSpeed
 // TODO: use switch_config.PortSpeed
 8: string portMaxSpeed
 // TODO: use switch_config.PortFEC?
 9: string portFEC
 // TODO: use switch_config.PortPause?
 10: bool rxPause
 11: bool txPause
 12: map<string, VlanInfo> vlanMemberShips
 13: i32 sFlowIngressRate
 14: i32 sFlowEgressRate
 15: list<PortQueueFields> queues
 16: string portLoopbackMode
 17: optional string ingressMirror
 18: optional string egressMirror
 19: optional string qosPolicy
 20: optional string sampleDest
 // TODO: this will deprecate port speed and port fec
 21: string portProfileID
 22: list<switch_config.AclLookupClass> lookupClassesToDistrubuteTrafficOn
 23: i32 maxFrameSize = switch_config.DEFAULT_PORT_MTU
}
