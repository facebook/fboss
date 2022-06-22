#
# Copyright 2004-present Facebook. All Rights Reserved.
#
namespace py neteng.fboss.switch_state
namespace py3 neteng.fboss
namespace py.asyncio neteng.fboss.asyncio.switch_state
namespace cpp2 facebook.fboss.state
namespace go neteng.fboss.switch_state

include "fboss/agent/switch_config.thrift"
include "fboss/lib/phy/phy.thrift"
include "fboss/agent/if/common.thrift"
include "fboss/qsfp_service/if/transceiver.thrift"
include "common/network/if/Address.thrift"

struct VlanInfo {
  1: bool tagged;
}

struct BufferPoolFields {
  1: string id;
  2: i32 headroomBytes;
  3: i32 sharedBytes;
}

struct PortPgFields {
  1: i16 id;
  2: i32 minLimitBytes;
  3: optional i32 headroomLimitBytes;
  4: optional string name;
  5: optional i32 resumeOffsetBytes;
  6: string bufferPoolName;
  7: optional string scalingFactor;
  8: optional BufferPoolFields bufferPoolConfig;
}

// Port queueing configuration
struct PortQueueFields {
  1: i16 id;
  2: i32 weight;
  3: optional i32 reserved;
  // TODO: replace with switch_config.MMUScalingFactor?
  4: optional string scalingFactor;
  // TODO: replace with switch_config.QueueScheduling?
  5: string scheduling;
  // TODO: replace with switch_config.StreamType?
  6: string streamType;
  7: optional list<switch_config.ActiveQueueManagement> aqms;
  8: optional string name;
  /*
  * Refer PortQueueRate which is a generalized version and allows configuring
  * pps as well as kbps.
  */
  10: optional i32 packetsPerSec_DEPRECATED;
  11: optional i32 sharedBytes;
  12: optional switch_config.PortQueueRate portQueueRate;

  13: optional i32 bandwidthBurstMinKbits;
  14: optional i32 bandwidthBurstMaxKbits;
  15: optional i16 trafficClass;
  16: optional list<i16> pfcPriorities;
}

// Port configuration and oper state fields
// TODO: separate config and operational state
struct PortFields {
  1: required i32 portId;
  2: required string portName;
  3: string portDescription;
  // TODO: use switch_config.PortState?
  4: string portState;
  5: bool portOperState;
  6: i32 ingressVlan;
  // TODO: use switch_config.PortSpeed?
  7: string portSpeed;
  // TODO: use switch_config.PortPause?
  10: bool rxPause;
  11: bool txPause;
  12: map<string, VlanInfo> vlanMemberShips;
  13: i32 sFlowIngressRate;
  14: i32 sFlowEgressRate;
  15: list<PortQueueFields> queues;
  16: string portLoopbackMode;
  17: optional string ingressMirror;
  18: optional string egressMirror;
  19: optional string qosPolicy;
  20: optional string sampleDest;
  // TODO: this will deprecate port speed and port fec
  21: string portProfileID;
  22: list<switch_config.AclLookupClass> lookupClassesToDistrubuteTrafficOn;
  23: i32 maxFrameSize = switch_config.DEFAULT_PORT_MTU;
  24: optional switch_config.PortPfc pfc;
  25: optional list<PortPgFields> pgConfigs;
  // TODO: Current warmboot state doesn't have such field yet
  26: optional phy.ProfileSideConfig profileConfig;
  27: optional list<phy.PinConfig> pinConfigs;
  28: optional phy.ProfileSideConfig lineProfileConfig;
  29: optional list<phy.PinConfig> linePinConfigs;
}

struct TransceiverSpecFields {
  1: required i16 id;
  2: optional double cableLength;
  3: optional transceiver.MediaInterfaceCode mediaInterface;
  4: optional transceiver.TransceiverManagementInterface managementInterface;
}

struct AclTtl {
  1: i16 value;
  2: i16 mask;
}

struct SendToQueue {
  1: switch_config.QueueMatchAction action;
  2: bool sendToCPU;
}

struct RedirectToNextHopAction {
  1: switch_config.RedirectToNextHopAction action;
  2: list<common.NextHopThrift> resolvedNexthops;
}

struct MatchAction {
  1: optional SendToQueue sendToQueue;
  2: optional switch_config.TrafficCounter trafficCounter;
  3: optional switch_config.SetDscpMatchAction setDscp;
  4: optional string ingressMirror;
  5: optional string egressMirror;
  6: optional switch_config.ToCpuAction toCpuAction;
  7: optional switch_config.MacsecFlowAction macsecFlow;
  8: optional RedirectToNextHopAction redirectToNextHop;
}

struct AclEntryFields {
  1: i32 priority;
  2: string name;
  // srcIp and dstIp need to be in CIDRNetwork format
  3: optional string srcIp;
  4: optional string dstIp;
  5: optional byte proto;
  6: optional byte tcpFlagsBitMap;
  7: optional i16 srcPort;
  8: optional i16 dstPort;
  9: optional switch_config.IpFragMatch ipFrag;
  10: optional byte icmpType;
  11: optional byte icmpCode;
  12: optional byte dscp;
  13: optional switch_config.IpType ipType;
  14: optional AclTtl ttl;
  15: optional string dstMac;
  16: optional i16 l4SrcPort;
  17: optional i16 l4DstPort;
  18: optional switch_config.AclLookupClass lookupClassL2;
  19: optional switch_config.AclLookupClass lookupClass;
  20: optional switch_config.AclLookupClass lookupClassNeighbor;
  21: optional switch_config.AclLookupClass lookupClassRoute;
  22: optional switch_config.PacketLookupResultType packetLookupResult;
  23: optional switch_config.EtherType etherType;
  24: switch_config.AclActionType actionType = switch_config.AclActionType.PERMIT;
  25: optional MatchAction aclAction;
  26: optional i32 vlanID;
  27: optional bool enabled;
}

enum NeighborState {
  Unverified = 0,
  Pending = 1,
  Reachable = 2,
}

struct NeighborEntryFields {
  1: string ipaddress;
  2: string mac;
  3: switch_config.PortDescriptor portId;
  4: i32 interfaceId;
  5: NeighborState state;
  6: optional switch_config.AclLookupClass classID;
  7: optional i64 encapIndex;
  8: bool isLocal = true;
}

enum MacEntryType {
  DYNAMIC_ENTRY = 0,
  STATIC_ENTRY = 1,
}

struct MacEntryFields {
  1: string mac;
  2: switch_config.PortDescriptor portId;
  3: optional switch_config.AclLookupClass classID;
  4: MacEntryType type;
}

struct NeighborResponseEntryFields {
  1: string ipAddress;
  2: string mac;
  3: i32 interfaceId;
}

struct VlanFields {
  1: i16 vlanId;
  2: string vlanName;
  3: i32 intfID;
  4: string dhcpV4Relay;
  5: string dhcpV6Relay;
  6: map<string, string> dhcpRelayOverridesV4;
  7: map<string, string> dhcpRelayOverridesV6;
  8: map<i16, bool> ports;
  9: map<string, NeighborEntryFields> arpTable;
  10: map<string, NeighborResponseEntryFields> arpResponseTable;
  11: map<string, NeighborEntryFields> ndpTable;
  12: map<string, NeighborResponseEntryFields> ndpResponseTable;
  13: map<string, MacEntryFields> macTable;
}

struct LoadBalancerFields {
  1: switch_config.LoadBalancerID id;
  2: switch_config.HashingAlgorithm algorithm;
  3: i64 seed;
  4: set<switch_config.IPv4Field> v4Fields;
  5: set<switch_config.IPv6Field> v6Fields;
  6: set<switch_config.TransportField> transportFields;
  7: set<switch_config.MPLSField> mplsFields;
}

struct MirrorTunnel {
  1: Address.BinaryAddress srcIp;
  2: Address.BinaryAddress dstIp;
  3: string srcMac;
  4: string dstMac;
  5: optional i16 udpSrcPort;
  6: optional i16 udpDstPort;
  7: i16 ttl = 255;
}

struct MirrorFields {
  1: string name;
  3: i16 dscp;
  4: bool truncate;
  5: bool configHasEgressPort;
  6: optional i32 egressPort;
  7: optional Address.BinaryAddress destinationIp;
  8: optional Address.BinaryAddress srcIp;
  9: optional i16 udpSrcPort;
  10: optional i16 udpDstPort;
  11: optional MirrorTunnel tunnel;
  12: bool isResolved;
}

struct ControlPlaneFields {
  1: list<PortQueueFields> queues;
  2: list<switch_config.PacketRxReasonToQueue> rxReasonToQueue;
  3: optional string defaultQosPolicy;
}

struct BlockedNeighbor {
  1: i16 blockNeighborVlanID;
  2: Address.BinaryAddress blockNeighborIP;
}

struct BlockedMacAddress {
  1: i16 macAddrToBlockVlanID;
  2: string macAddrToBlockAddr;
}

struct SwitchSettingsFields {
  1: switch_config.L2LearningMode l2LearningMode;
  2: bool qcmEnable;
  3: bool ptpTcEnable;
  4: i32 l2AgeTimerSeconds;
  5: i32 maxRouteCounterIDs;
  6: list<BlockedNeighbor> blockNeighbors;
  7: list<BlockedMacAddress> macAddrsToBlock;
}

struct RoutePrefix {
  1: bool v6;
  2: Address.BinaryAddress prefix;
  3: byte mask;
}

struct SwitchState {
  1: map<i16, PortFields> portMap;
  2: map<i16, VlanFields> vlanMap;
  3: map<string, AclEntryFields> aclMap;
  4: map<i16, TransceiverSpecFields> transceiverMap;
  5: map<string, BufferPoolFields> bufferPoolCfgMap;
  6: map<string, MirrorFields> mirrorMap;
  7: ControlPlaneFields controlPlane;
  8: SwitchSettingsFields switchSettings;
  9: i16 defaultVlan;
  10: i64 arpTimeout;
  11: i64 ndpTimeout;
  12: i32 arpAgerInterval;
  13: i32 maxNeighborProbes;
  14: i64 staleEntryInterval;
  15: Address.BinaryAddress dhcpV4RelaySrc;
  16: Address.BinaryAddress dhcpV6RelaySrc;
  17: Address.BinaryAddress dhcpV4ReplySrc;
  18: Address.BinaryAddress dhcpV6ReplySrc;
  19: optional switch_config.PfcWatchdogRecoveryAction pfcWatchdogRecoveryAction;
}
