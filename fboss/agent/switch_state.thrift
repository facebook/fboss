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
include "fboss/agent/if/ctrl.thrift"
include "fboss/mka_service/if/mka_structs.thrift"

struct VlanInfo {
  1: bool tagged;
}

struct PortPgFields {
  1: i16 id;
  2: i32 minLimitBytes;
  3: optional i32 headroomLimitBytes;
  4: optional string name;
  5: optional i32 resumeOffsetBytes;
  6: string bufferPoolName;
  7: optional string scalingFactor;
  8: optional common.BufferPoolFields bufferPoolConfig;
}

struct MKASakKey {
  1: mka_structs.MKASci sci;
  2: i32 associationNum;
}

struct RxSak {
  1: MKASakKey sakKey;
  2: mka_structs.MKASak sak;
}

// Port configuration and oper state fields
// TODO: separate config and operational state
struct PortFields {
  1: i32 portId;
  2: string portName;
  3: string portDescription;
  // TODO: use switch_config.PortState?
  4: string portState = "DISABLED";
  // portOperState::
  //  false => port is DOWN
  //  true => port is UP
  5: bool portOperState = false;
  6: i32 ingressVlan;
  // TODO: use switch_config.PortSpeed?
  7: string portSpeed = "DEFAULT";
  // TODO: use switch_config.PortPause?
  10: bool rxPause;
  11: bool txPause;
  12: map<string, VlanInfo> vlanMemberShips;
  13: i32 sFlowIngressRate;
  14: i32 sFlowEgressRate;
  15: list<ctrl.PortQueueFields> queues;
  16: string portLoopbackMode = "NONE";
  17: optional string ingressMirror;
  18: optional string egressMirror;
  19: optional string qosPolicy;
  20: optional string sampleDest;
  // TODO: this will deprecate port speed and port fec
  21: string portProfileID = "PROFILE_DEFAULT";
  22: list<switch_config.AclLookupClass> lookupClassesToDistrubuteTrafficOn;
  23: i32 maxFrameSize = switch_config.DEFAULT_PORT_MTU;
  24: optional switch_config.PortPfc pfc;
  25: optional list<PortPgFields> pgConfigs;
  // TODO: Current warmboot state doesn't have such field yet
  26: phy.ProfileSideConfig profileConfig;
  27: list<phy.PinConfig> pinConfigs;
  28: optional phy.ProfileSideConfig lineProfileConfig;
  29: optional list<phy.PinConfig> linePinConfigs;
  30: switch_config.PortType portType = switch_config.PortType.INTERFACE_PORT;
  31: optional phy.LinkFaultStatus iPhyLinkFaultStatus;
  32: phy.PortPrbsState asicPrbs;
  33: phy.PortPrbsState gbSystemPrbs;
  34: phy.PortPrbsState gbLinePrbs;
  35: optional list<i16> pfcPriorities;
  36: map<switch_config.LLDPTag, string> expectedLLDPValues;
  37: list<RxSak> rxSecureAssociationKeys;
  38: optional mka_structs.MKASak txSecureAssociationKey;
  39: bool macsecDesired = false;
  40: bool dropUnencrypted = false;
  // List of interfaces for given port:
  //
  // For the systems with VLAN based RIFs (non-VOQ/non-FABRIC):
  //   - A port could be part of multiple VLANs.
  //   - every VLAN corresponds to an interface.
  //   - interfaceIDs contains the list of interfaces for VLANs the port is
  //     part of.
  //   - In practice, a port is only part of single VLAN, so this vector always
  //     has size of 1.
  //
  // For the systems with Port based RIFs (VOQ/FABRIC switches):
  //   - interfaceIDs contains single element viz. the interface corresponding
  //     to this port.
  41: list<i32> interfaceIDs;
  42: list<switch_config.PortNeighbor> expectedNeighborReachability;
  43: switch_config.PortDrainState drainState = switch_config.PortDrainState.UNDRAINED;
  44: optional string flowletConfigName;
  45: optional PortFlowletFields flowletConfig;
  46: optional ctrl.PortLedExternalState portLedExternalState;
  47: bool rxLaneSquelch = false;
  48: bool zeroPreemphasis = false;

  // Set only for ASICs that distinguish UP from ACTIVE e.g. J2, J3 etc.
  // On those ASICs, an UP port is ACTIVE only if bi-directional connectivity
  // is established and ports on both sides are ready to send data traffic.
  //
  // When set, portActiveState::
  //  false => port is INACTIVE
  //  true => port is ACTIVE
  //
  // When portActiveState is set,
  //  - if portOperState is DOWN, portActiveState is always INACTIVE
  //  - if portOperState is UP, portActiveState is either ACTIVE or INACTIVE.
  49: optional bool portActiveState;
  50: optional bool disableTTLDecrement;
  51: optional bool txEnable;
  // Current active errors seen on port
  52: list<ctrl.PortError> activeErrors;
  53: switch_config.Scope scope = switch_config.Scope.LOCAL;
  54: optional i32 reachabilityGroupId;
}

typedef ctrl.SystemPortThrift SystemPortFields

struct TransceiverSpecFields {
  1: i16 id;
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

struct SetTc {
  1: switch_config.SetTcAction action;
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
  9: optional SetTc setTc;
  10: optional switch_config.UserDefinedTrapAction userDefinedTrap;
  11: optional switch_config.FlowletAction flowletAction;
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
  28: optional list<string> udfGroups;
  29: optional byte roceOpcode;
  30: optional list<byte> roceBytes;
  31: optional list<byte> roceMask;
  32: optional list<switch_config.AclUdfEntry> udfTable;
}

enum NeighborState {
  Unverified = 0,
  Pending = 1,
  Reachable = 2,
}

enum NeighborEntryType {
  DYNAMIC_ENTRY = 0,
  STATIC_ENTRY = 1,
}

struct NeighborEntryFields {
  1: string ipaddress;
  2: string mac = "ff:ff:ff:ff:ff:ff";
  3: switch_config.PortDescriptor portId;
  4: i32 interfaceId;
  5: NeighborState state = NeighborState.Reachable;
  6: optional switch_config.AclLookupClass classID;
  7: optional i64 encapIndex;
  8: bool isLocal = true;
  9: NeighborEntryType type = NeighborEntryType.DYNAMIC_ENTRY;
  10: optional i64 resolvedSince;
  11: optional bool noHostRoute;
  12: optional bool disableTTLDecrement;
}

typedef map<string, NeighborEntryFields> NeighborEntries

enum MacEntryType {
  DYNAMIC_ENTRY = 0,
  STATIC_ENTRY = 1,
}

struct MacEntryFields {
  1: string mac;
  2: switch_config.PortDescriptor portId;
  3: optional switch_config.AclLookupClass classID;
  4: MacEntryType type = MacEntryType.DYNAMIC_ENTRY;
}

struct NeighborResponseEntryFields {
  1: string ipAddress;
  2: string mac;
  3: i32 interfaceId;
}

struct VlanFields {
  1: i16 vlanId = 0;
  2: string vlanName;
  3: i32 intfID = 0;
  4: string dhcpV4Relay = "0.0.0.0";
  5: string dhcpV6Relay = "::";
  6: map<string, string> dhcpRelayOverridesV4;
  7: map<string, string> dhcpRelayOverridesV6;
  8: map<i16, bool> ports;
  9: NeighborEntries arpTable;
  10: map<string, NeighborResponseEntryFields> arpResponseTable;
  11: NeighborEntries ndpTable;
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
  8: list<string> udfGroups = [];
}

struct MirrorTunnel {
  1: Address.BinaryAddress srcIp;
  2: Address.BinaryAddress dstIp;
  3: string srcMac;
  4: string dstMac;
  5: optional i16 udpSrcPort;
  6: optional i16 udpDstPort;
  7: i16 ttl = 127;
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
  13: i64 switchId;
  14: optional switch_config.PortDescriptor egressPortDesc;
  15: optional i32 samplingRate;
}

struct ControlPlaneFields {
  1: list<ctrl.PortQueueFields> queues;
  2: list<switch_config.PacketRxReasonToQueue> rxReasonToQueue;
  3: optional string defaultQosPolicy;
}

struct PortFlowletFields {
  1: string id;
  2: i16 scalingFactor;
  3: i16 loadWeight;
  4: i16 queueWeight;
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
  1: switch_config.L2LearningMode l2LearningMode = switch_config.L2LearningMode.HARDWARE;
  2: bool qcmEnable = false;
  3: bool ptpTcEnable = false;
  4: i32 l2AgeTimerSeconds = 300;
  5: i32 maxRouteCounterIDs = 0;
  6: list<BlockedNeighbor> blockNeighbors;
  7: list<BlockedMacAddress> macAddrsToBlock;
  // Switch type
  8: switch_config.SwitchType switchType_DEPRECATED;
  9: optional i64 switchId_DEPRECATED;
  10: list<switch_config.ExactMatchTableConfig> exactMatchTableConfigs;
  11: optional switch_config.Range64 systemPortRange_DEPRECATED;
  12: optional i16 defaultVlan;
  13: optional i64 arpTimeout;
  14: optional i64 ndpTimeout;
  15: optional i32 arpAgerInterval;
  16: optional i32 maxNeighborProbes;
  17: optional i64 staleEntryInterval;
  18: optional Address.BinaryAddress dhcpV4RelaySrc;
  19: optional Address.BinaryAddress dhcpV6RelaySrc;
  20: optional Address.BinaryAddress dhcpV4ReplySrc;
  21: optional Address.BinaryAddress dhcpV6ReplySrc;
  23: optional QcmCfgFields qcmCfg;
  24: optional QosPolicyFields defaultDataPlaneQosPolicy;
  25: optional switch_config.UdfConfig udfConfig;
  26: optional switch_config.FlowletSwitchingConfig flowletSwitchingConfig;
  27: map<i64, switch_config.SwitchType> switchIdToSwitchType_DEPRECATED;
  28: switch_config.SwitchDrainState switchDrainState = switch_config.SwitchDrainState.UNDRAINED;
  29: map<i64, switch_config.SwitchInfo> switchIdToSwitchInfo;
  30: optional i32 minLinksToRemainInVOQDomain;
  31: optional i32 minLinksToJoinVOQDomain;
  32: switch_config.SwitchDrainState actualSwitchDrainState = switch_config.SwitchDrainState.UNDRAINED;
  33: list<ctrl.PortQueueFields> defaultVoqConfig;
  34: switch_config.SwitchInfo switchInfo;
  // MAC OUIs (24-bit MAC address prefix) used by vendor NICs.
  // When queue-per-host is enabled, MACs matching any OUI from this list will get a dedicated queue.
  35: list<string> vendorMacOuis;
  // MAC OUIs used by meta for VM purpose.
  // When queue-per-host is enabled, MACs matching any OUI from this list could get any queue.
  36: list<string> metaMacOuis;
  37: ctrl.SwitchRunState swSwitchRunState;
  38: optional bool forceTrafficOverFabric;
  39: optional bool creditWatchdog;
  40: optional bool forceEcmpDynamicMemberUp;
  // Programmable hostname, useful for ICMP responses and the like.
  41: string hostname;
  // When there's no IPv4 addresses configured, what address to use to source IPv4 ICMP packets from.
  42: Address.BinaryAddress icmpV4UnavailableSrcAddress;
  // Switch property of reachability group size, for the use of input balanced mode.
  43: optional i32 reachabilityGroupListSize;
}

struct RoutePrefix {
  1: bool v6;
  2: Address.BinaryAddress prefix;
  3: byte mask;
}

struct RouteNextHopEntry {
  1: ctrl.AdminDistance adminDistance = ctrl.AdminDistance.MAX_ADMIN_DISTANCE;
  2: ctrl.RouteForwardAction action = ctrl.RouteForwardAction.DROP;
  3: optional string counterID;
  4: optional switch_config.AclLookupClass classID;
  5: list<common.NextHopThrift> nexthops;
}

struct RouteNextHopsMulti {
  1: ctrl.ClientID lowestAdminDistanceClientId;
  2: map<ctrl.ClientID, RouteNextHopEntry> client2NextHopEntry;
}

struct RouteFields {
  1: RoutePrefix prefix;
  2: RouteNextHopsMulti nexthopsmulti;
  3: RouteNextHopEntry fwd;
  4: i32 flags;
  5: optional switch_config.AclLookupClass classID;
}

struct Label {
  1: i32 value;
}

struct LabelForwardingEntryFields {
  1: Label label;
  2: RouteNextHopsMulti nexthopsmulti;
  3: RouteNextHopEntry fwd;
  4: i32 flags;
  5: optional switch_config.AclLookupClass classID;
}

struct FibContainerFields {
  1: i16 vrf;
  2: map<string, RouteFields> fibV4;
  3: map<string, RouteFields> fibV6;
}

struct TrafficClassToQosAttributeEntry {
  1: i16 trafficClass;
  2: i16 attr;
}

struct TrafficClassToQosAttributeMap {
  1: list<TrafficClassToQosAttributeEntry> from;
  2: list<TrafficClassToQosAttributeEntry> to;
}

struct IpTunnelFields {
  1: string ipTunnelId;
  2: i32 underlayIntfId;
  3: i32 mode; // Not used
  4: string dstIp;
  5: i32 type;
  6: i32 tunnelTermType;
  7: optional i32 ttlMode;
  8: optional i32 dscpMode;
  9: optional i32 ecnMode;
  10: optional string srcIp;
  11: optional string dstIpMask;
  12: optional string srcIpMask;
}

struct QosPolicyFields {
  1: string name;
  2: TrafficClassToQosAttributeMap dscpMap;
  3: TrafficClassToQosAttributeMap expMap;
  // TODO(zecheng): Use strong types when adapter is integrated.
  4: map<i16, i16> trafficClassToQueueId;
  5: optional map<i16, i16> pfcPriorityToQueueId;
  6: optional map<i16, i16> trafficClassToPgId;
  7: optional map<i16, i16> pfcPriorityToPgId;
  8: optional map<i16, i16> trafficClassToVoqId;
}

struct SocketAddress {
  1: string host;
  2: i32 port;
}

struct SflowCollectorFields {
  1: string id;
  2: SocketAddress address;
}

struct InterfaceFields {
  1: i32 interfaceId;
  2: i32 routerId;
  3: optional i32 vlanId;
  4: string name;
  // network byte order
  5: i64 mac;
  // ip -> prefix length
  6: map<string, i16> addresses;
  7: switch_config.NdpConfig ndpConfig;
  8: i32 mtu;
  9: bool isVirtual = false;
  10: bool isStateSyncDisabled = false;
  11: switch_config.InterfaceType type = switch_config.InterfaceType.VLAN;
  12: NeighborEntries arpTable;
  13: NeighborEntries ndpTable;
  14: map<string, NeighborResponseEntryFields> arpResponseTable;
  15: map<string, NeighborResponseEntryFields> ndpResponseTable;
  16: optional string dhcpV4Relay;
  17: optional string dhcpV6Relay;
  18: map<string, string> dhcpRelayOverridesV4;
  19: map<string, string> dhcpRelayOverridesV6;

  /*
   * Set only on Remote Interfaces of VOQ switches.
   */
  20: optional common.RemoteInterfaceType remoteIntfType;

  /*
   * Set only on Remote Interfaces of VOQ switches.
   */
  21: optional common.LivenessStatus remoteIntfLivenessStatus;
  22: switch_config.Scope scope = switch_config.Scope.LOCAL;
}

enum LacpState {
  NONE = 0,
  ACTIVE = 1,
  SHORT_TIMEOUT = 2,
  AGGREGATABLE = 4,
  IN_SYNC = 8,
  COLLECTING = 16,
  DISTRIBUTING = 32,
  DEFAULTED = 64,
  EXPIRED = 128,
}

struct ParticipantInfo {
  1: i32 systemPriority;
  2: list<i16> systemID;
  3: i32 key;
  4: i32 portPriority;
  5: i32 port;
  6: LacpState state;
}

struct Subport {
  1: i32 id = 0;
  2: i32 priority = 0;
  3: switch_config.LacpPortRate lacpPortRate = switch_config.LacpPortRate.SLOW;
  4: switch_config.LacpPortActivity lacpPortActivity = switch_config.LacpPortActivity.PASSIVE;
  5: i32 holdTimerMultiplier = switch_config.DEFAULT_LACP_HOLD_TIMER_MULTIPLIER;
}

struct AggregatePortFields {
  1: i16 id;
  2: string name;
  3: string description;
  4: i32 systemPriority;
  // network byte order
  5: i64 systemID;
  6: i16 minimumLinkCount;
  7: list<Subport> ports;
  // portId to forwarding {ture -> enabled; false -> disabled};
  8: map<i32, bool> portToFwdState;
  // PortId to ParticipantInfo struct
  9: map<i32, ParticipantInfo> portToPartnerState;
  // List of interfaces for given aggregate port
  10: list<i32> interfaceIDs;
}

struct TeFlowEntryFields {
  1: ctrl.TeFlow flow;
  3: list<common.NextHopThrift> nexthops;
  4: list<common.NextHopThrift> resolvedNexthops;
  5: bool enabled;
  6: optional ctrl.TeCounterID counterID;
  7: optional bool statEnabled;
}

struct AclTableFields {
  1: string id;
  2: i32 priority;
  3: optional map<string, AclEntryFields> aclMap;
  4: list<switch_config.AclTableActionType> actionTypes;
  5: list<switch_config.AclTableQualifier> qualifiers;
}

struct AclTableGroupFields {
  1: switch_config.AclStage stage;
  2: string name;
  3: optional map<string, AclTableFields> aclTableMap;
}

struct QcmCfgFields {
  1: i32 agingIntervalInMsecs = switch_config.DEFAULT_QCM_AGING_INTERVAL_MSECS;
  2: i32 numFlowSamplesPerView = switch_config.DEFAULT_QCM_FLOWS_PER_VIEW;
  3: i32 flowLimit = switch_config.DEFAULT_QCM_FLOW_LIMIT;
  4: i32 numFlowsClear = switch_config.DEFAULT_QCM_NUM_FLOWS_TO_CLEAR;
  5: i32 scanIntervalInUsecs = switch_config.DEFAULT_QCM_SCAN_INTERVAL_USECS;
  6: i32 exportThreshold = switch_config.DEFAULT_QCM_EXPORT_THRESHOLD;
  7: bool monitorQcmCfgPortsOnly = false;
  8: map<i32, i32> flowWeights;
  // srcIp and dstIp need to be in CIDRNetwork format
  9: ctrl.IpPrefix collectorSrcIp;
  10: ctrl.IpPrefix collectorDstIp;
  11: i16 collectorSrcPort;
  12: i16 collectorDstPort;
  13: optional byte collectorDscp;
  14: optional i32 ppsToQcm;
  15: list<i32> monitorQcmPortList;
  16: map<i32, set<i32>> port2QosQueueIds;
}

// String encoding SwitchId list for indexing multi switch tables.
// eg: "Id:124,125,130" indicates a table applicable to SwitchIds 124, 125 and 130
typedef string SwitchIdList

struct SwitchState {
  100: map<SwitchIdList, map<i16, PortFields>> portMaps;
  101: map<SwitchIdList, map<i16, VlanFields>> vlanMaps;
  102: map<SwitchIdList, map<string, AclEntryFields>> aclMaps;
  103: map<SwitchIdList, map<i16, TransceiverSpecFields>> transceiverMaps;
  104: map<
    SwitchIdList,
    map<string, common.BufferPoolFields>
  > bufferPoolCfgMaps;
  105: map<SwitchIdList, map<string, MirrorFields>> mirrorMaps;
  106: map<SwitchIdList, ControlPlaneFields> controlPlaneMap;
  107: map<SwitchIdList, SwitchSettingsFields> switchSettingsMap;
  108: map<SwitchIdList, map<i64, SystemPortFields>> systemPortMaps;
  109: map<SwitchIdList, map<i16, FibContainerFields>> fibsMap;
  110: map<SwitchIdList, map<i32, LabelForwardingEntryFields>> labelFibMap;
  111: map<SwitchIdList, map<string, QosPolicyFields>> qosPolicyMaps;
  112: map<SwitchIdList, map<string, SflowCollectorFields>> sflowCollectorMaps;
  113: map<SwitchIdList, map<string, IpTunnelFields>> ipTunnelMaps;
  114: map<SwitchIdList, map<string, TeFlowEntryFields>> teFlowTables;
  115: map<SwitchIdList, map<i16, AggregatePortFields>> aggregatePortMaps;
  116: map<
    SwitchIdList,
    map<switch_config.LoadBalancerID, LoadBalancerFields>
  > loadBalancerMaps;
  117: map<
    SwitchIdList,
    map<switch_config.AclStage, AclTableGroupFields>
  > aclTableGroupMaps;
  118: map<SwitchIdList, map<i32, InterfaceFields>> interfaceMaps;
  119: map<SwitchIdList, map<i64, switch_config.DsfNode>> dsfNodesMap;
  120: map<SwitchIdList, map<string, PortFlowletFields>> portFlowletCfgMaps;
  // Remote object maps
  600: map<SwitchIdList, map<i64, SystemPortFields>> remoteSystemPortMaps;
  601: map<SwitchIdList, map<i32, InterfaceFields>> remoteInterfaceMaps;
} (thriftpath.root)

struct RouteTableFields {
  1: map<string, RouteFields> v4NetworkToRoute;
  2: map<string, RouteFields> v6NetworkToRoute;
  3: map<i32, LabelForwardingEntryFields> labelToRoute;
}

struct WarmbootState {
  1: SwitchState swSwitchState;
  2: map<i32, RouteTableFields> routeTables;
// TODO: Extend for hwSwitchState
}
