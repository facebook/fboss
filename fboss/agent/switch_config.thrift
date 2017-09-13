#
# Copyright 2004-present Facebook. All Rights Reserved.
#
namespace py neteng.fboss.switch_config
namespace py.asyncio neteng.fboss.asyncio.switch_config
namespace cpp2 facebook.fboss.cfg


/**
 * Administrative port state. ie "should the port be enabled?"
 */
enum PortState {
  // Deprecated from when this capture administrative and operational state.
  DOWN = 0,

  DISABLED = 1,  // Disabled, no signal transmitted
  ENABLED = 2,
}

/**
 * The parser type for a port.
 * This controls how much of the packet header will be parsed.
 */
enum ParserType {
  L2 = 0,
  L3 = 1,
  L4 = 2,
  ALL = 3,
}

/**
 * The spanning tree state for a VlanPort.
 *
 * This maps to the FocalPoint API's FM_STP_STATE_* constants.
 */
enum SpanningTreeState {
  BLOCKING = 0,   // Receive BPDUs only, no traffic
  DISABLED = 1,   // Receive nothing
  FORWARDING = 2, // Process BDPUs and traffic
  LEARNING = 3,   // Send and receive BDPUs, but no traffic
  LISTENING = 4,  // Send and receive BPDUs, but no learning or traffic
}

/**
 * The speed value for a port
 */
enum PortSpeed {
  DEFAULT = 0;                  // Default for that port defined by HW
  GIGE = 1000;                  // Gig Ethernet
  XG = 10000;                   // 10G
  TWENTYG = 20000;              // 20G
  TWENTYFIVEG = 25000;          // 25G
  FORTYG = 40000;               // 40G
  FIFTYG = 50000;               // 50G
  HUNDREDG = 100000;            // 100G
}

/**
 * The pause setting for a port
 */
struct PortPause {
  1: bool tx = false
  2: bool rx = false
}

/**
 *  A Range for L4 port range checker
 *  Define a range bewteen [min, max]
 */
struct L4PortRange {
  1: i32 min
  2: i32 max
}

/**
 *  A Range for packet length
 *  Define a packet length range [min, max]
 */
struct PktLenRange {
  1: i16 min
  2: i16 max
}

enum IpFragMatch {
  // not fragment
  MATCH_NOT_FRAGMENTED = 0
  // first fragment
  MATCH_FIRST_FRAGMENT = 1
  // not fragment or the first fragment
  MATCH_NOT_FRAGMENTED_OR_FIRST_FRAGMENT = 2
  // fragment but not the first frament
  MATCH_NOT_FIRST_FRAGMENT = 3
  // any fragment
  MATCH_ANY_FRAGMENT = 4
}

/**
 * The action for an access control entry
 */
enum AclActionType {
  DENY = 0
  PERMIT = 1
}

/**
 * An access control entry
 */
struct AclEntry {
  /**
   * IP addresses with mask. e.g. 192.168.0.0/16. Can be either V4 or V6
   */
  3: optional string srcIp
  4: optional string dstIp

  /**
   * L4 port ranges (TCP/UDP)
   */
  5: optional L4PortRange srcL4PortRange
  6: optional L4PortRange dstL4PortRange

  /**
   * IP Protocol. e.g, 6 for TCP
   */
  7: optional i16 proto

  /**
   * TCP flags and mask (to support "don't care" bits). As in IP address,
   * mask = 1 means we _care_ about this bit position.
   * Example: tcpFlags = 16, tcpFlagsMask = 16 means ACK set, while ignoring
   * all other bits
   */
  8: optional i16 tcpFlags
  9: optional i16 tcpFlagsMask

  /**
   * Physical switch ports.
   */
  10: optional i16 srcPort
  11: optional i16 dstPort

  /**
   * Packet length range
   */
  12: optional PktLenRange pktLenRange

  /**
   * Ip fragment
   */
  13: optional IpFragMatch ipFrag

  /**
   * Icmp type and code
   * Code can only be set if type is set.
   * "proto" field must be 1 (icmpv4) or 58 (icmpv6)
   */
  14: optional i16 icmpType
  15: optional i16 icmpCode
  /*
  * Match on DSCP values
  */
  16: optional byte dscp
  /*
   * Identifier used to refer to the ACL
   */
  17: string name

  18: AclActionType actionType = PERMIT
  // This is temporary and will be removed in the next diff that introduces QoS
  19: optional i16 qosQueueNum
}

/*
 * We only support unicast in FBOSS, but for completeness sake
 */
enum StreamType {
  UNICAST = 0,
  MULTICAST = 1,
}

struct QueueMatchAction {
  1: i16 queueId
}

struct MatchAction {
  1: optional QueueMatchAction sendToQueue
  2: bool sendToCPU = false
}

struct MatchToAction {
  // This references an ACL added to the acls list in the SwitchConfig object
  1: string matcher
  2: MatchAction action
}

enum MMUScalingFactor {
  ONE = 0
  EIGHT = 1
}

struct PortQueue {
  1: required i16 id
  // We only use unicast in Fabric
  2: required StreamType streamType = StreamType.UNICAST
  3: optional i32 weight
  4: optional i32 reservedBytes
  5: optional MMUScalingFactor scalingFactor
}

struct TrafficPolicyConfig {
  // Order of entries determines priority of acls when applied
  1: list<MatchToAction> matchToAction = []
}

/**
 * Configuration for a single logical port
 */
struct Port {
  1: i32 logicalID
  /*
   * Whether the port is enabled.  Set this to ENABLED for normal operation.
   */
  2: PortState state = DISABLED
  /*
   * Packets ingressing on this port will be dropped if they are smaller than
   * minFrameSize.  64 bytes is the minimum frame size allowed by ethernet.
   */
  3: i32 minFrameSize = 64
  /*
   * Packets ingressing on this port will be dropped if they are larger than
   * maxFrameSize.  This should generally match the MTUs for the VLANs that
   * this port belongs to.
   */
  4: i32 maxFrameSize
  /*
   * This parameter controls how packets ingressing on this port will be
   * parsed.  If set to L2, only the L2 headers will be parsed.  You need to
   * set this to at least L3 to enable routing.  Set this to L4 if you need
   * features that examine TCP/UDP port numbers, such as ECMP.
   */
  5: ParserType parserType
  /*
   * This parameter must be set to true for packets ingressing on this port to
   * be routed.  If this is set to False, ingress traffic will only be switched
   * at layer 2.
   */
  6: bool routable
  /*
   * The VLAN ID for untagged packets ingressing on this port.
   */
  7: i32 ingressVlan
  /**
   * The speed of the interface in mbps.
   * Value 0 means default setting based on the HW and port type.
   */
  8: PortSpeed speed = DEFAULT;

  /**
   * A configurable string describing the name of the port. If this
   * is not set, it will default to 'port-logicalID'
   */
  9: optional string name

  /**
   * An optional configurable string describing the port.
   */
  10: optional string description

  /**
   * If this is undefined, the global TrafficPolicyConfig will be used
   */
  11: optional TrafficPolicyConfig egressTrafficPolicy

  /*
   * There are multiple queues per port
   * This allows defining their attributes
   */
  12: list<PortQueue> queues = []

  /**
   * pause configuration
   */
  13: PortPause pause

  /**
   * sFlow sampling rate for ingressing packets.
   * Every 1/sFlowIngressRate ingressing packets will be sampled.
   * 0 indicates no sampling while 1 indicates sampling all packets.
   */
  14: i64 sFlowIngressRate = 0;

  /**
   * sFlow sampling rate for egressing packets.
   * Every 1/sFlowEgressRate egressing packets will be sampled.
   * 0 indicates no sampling while 1 indicates sampling all packets.
   */
  15: i64 sFlowEgressRate = 0;
}

struct AggregatePort {
  1: i16 key
  2: string name
  3: string description
  /**
   * Physical ports are identified here according to their logicalID,
   * as set in struct Port.
   */
  4: list<i32> physicalPorts
}

/**
 * Configuration for a VLAN.
 *
 * For simplicity, we use a single list of VLANs for the entire controller
 * process.  The FocalPoint API has an independent list of VLANs for each
 * switch.  The controller will create each VLAN on each configured switch.
 */
struct Vlan {
  1: string name
  2: i32 id
  /**
   * Whether or not to enable stats collection for this VLAN in hardware.
   * (The hardware is only able to collect stats on a limited number of VLANs.)
   */
  3: bool recordStats = 1
  /**
   * Whether packets ingressing on this VLAN should be routed.
   * When this is false, ingressing traffic will only be switched at layer 2.
   *
   * Note that you must also set the routable attribute to true on each port in
   * the vlan.
   */
  5: bool routable
  /**
   * IP addresses associated with this interface.
   *
   * IP addresses are only useful if the routable flag is set to true for this
   * VLAN.
   */
  6: list<string> ipAddresses
  /* V4 DHCP relay address */
  7: optional string dhcpRelayAddressV4
  /* V6 DHCP relay address */
  8: optional string dhcpRelayAddressV6

  /* Override DHCPv4/6 relayer on a per host basis */
  9: optional map<string, string> dhcpRelayOverridesV4
  10: optional map<string, string> dhcpRelayOverridesV6

  /* Interface ID associated with the vlan */
  11: optional i32 intfID
}

/**
 * A VLAN <--> Port mapping.
 *
 * The specified logical port will be added to the specified VLAN.
 * The VlanPort structure is separate from Port since a single Port may belong
 * to multiple VLANs.
 */
struct VlanPort {
  1: i32 vlanID
  2: i32 logicalPort
  3: SpanningTreeState spanningTreeState
  /**
   * Whether or not to emit the VLAN tag in outgoing ethernet frames.
   * (Primarily useful when there are multiple VLANs on the same port.)
   */
  4: bool emitTags
}

/**
 * IPv6 NDP configuration settings for an interface
 */
struct NdpConfig {
  /**
   * How often to send out router advertisements, in seconds.
   *
   * If this is 0 or negative, no router advertisements will be sent.
   */
  1: i32 routerAdvertisementSeconds = 0
  /**
   * The recommended hop limit that hosts should put in outgoing IPv6 packets.
   *
   * This value may be between 0 and 255, inclusive.
   * 0 means unspecified.
   */
  2: i32 curHopLimit = 255
  /**
   * The router lifetime to send in advertisement messasges.
   *
   * This should generally be larger than the router advertisement interval.
   */
  3: i32 routerLifetime = 1800
  /**
   * The lifetime to advertise for IPv6 prefixes on this interface.
   */
  4: i32 prefixValidLifetimeSeconds = 2592000
  /**
   * The preferred lifetime to advertise for IPv6 prefixes on this interface.
   *
   * This must be smaller than prefixValidLifetimeSeconds
   */
  5: i32 prefixPreferredLifetimeSeconds = 604800
  /**
  * Managed bit setting for router advertisements
  * When set, it indicates that addresses are available via
  * Dynamic Host Configuration Protocol
  */
  6: bool routerAdvertisementManagedBit = 0
  /**
  * Other bit setting for router advertisements
  * When set, it indicates that other configuration information is
  * available via DHCPv6.  Examples of such information
  * are DNS-related information or information on other
  * servers within the network.
  */
  7: bool routerAdvertisementOtherBit = 0;
}

/**
 * The configuration for an interface
 */
struct Interface {
  1: i32 intfID
  /** The VRF where the interface belongs to */
  2: i32 routerID = 0
  /**
   * The VLAN where the interface sits.
   * There shall be maximum one interface per VLAN
   */
  3: i32 vlanID
  /** The description of the interface */
  4: optional string name
  /**
   * The MAC address to be used by this interface.
   * If it is not specified here, it is up to the platform
   * to pick up a MAC for it.
   */
  5: optional string mac
  /**
   * IP addresses with netmask associated with this interface
   * in the format like: 10.0.0.0/8
   */
  6: list<string> ipAddresses
  /**
   * IPv6 NDP configuration
   */
  7: optional NdpConfig ndp

  /**
   * MTU size
   */
  8: optional i32 mtu
  /**
   * is_virtual is set to true for logical interfaces
   * (e.g. loopbacks) which are associated with
   * a reserver vlan. This VLAN has no ports in it
   * and the interface is expected to always be up.
   */
  9: bool isVirtual = 0
  /**
  * this flag is set to true if we need to
  * disable auto-state feature for SVI
  */
  10: optional bool isStateSyncDisabled = 0
}

struct StaticRouteWithNextHops {
  /** The VRF where the static route belongs to */
  1: i32 routerID = 0
  /* Prefix in the format like 10.0.0.0/8 */
  2: string prefix
  /* IP Address next hops like 1.1.1.1 */
  3: list<string> nexthops
}

struct StaticRouteNoNextHops {
  /** The VRF where the static route belongs to */
  1: i32 routerID = 0
  /* Prefix in the format like 10.0.0.0/8 */
  2: string prefix
}


struct SflowCollector {
  1: string ip
  2: i16 port
}

/**
 * The configuration for a switch.
 *
 * This contains all of the hardware-independent configuration for a
 * single switch/router in the network.
 */
struct SwitchConfig {
  1: i64 version = 0
  2: list<Port> ports
  3: list<Vlan> vlans = []
  4: list<VlanPort> vlanPorts = []
  /*
   * Broadcom chips have the notion of a "default" VLAN.
   * This VLAN must always exist.  It is used as the default VLAN for ingress
   * packets when a default ingress VLAN hasn't been set on a per-port basis.
   * We always set a per-port ingress VLAN, so this setting doesn't really have
   * much effect for us.
   */
  5: i32 defaultVlan
  6: list<Interface> interfaces = []
  7: i32 arpTimeoutSeconds = 60
  8: i32 arpRefreshSeconds = 20
  9: i32 arpAgerInterval = 5
  10: bool proactiveArp = 0
  // The MAC address to use for the switch CPU.
  11: optional string cpuMAC
  // Static routes with next hops
  12: optional list<StaticRouteWithNextHops> staticRoutesWithNhops = [];
  // Prefixes for which to drop traffic
  13: optional list<StaticRouteNoNextHops> staticRoutesToNull = [];
  // Prefixes for which to send traffic to CPU
  14: optional list<StaticRouteNoNextHops> staticRoutesToCPU = [];
  // These acls are not applied directly, instead they're used by TrafficPolicy
  // to define policy matchers and actions
  15: optional list<AclEntry> acls = []
  // Set max number of probes to a sufficiently high value
  // to allow for the cases where
  // a) We are probing and the agent on other end is restarting.
  // b) The other end is having a hard time and responses to ARP/NDP
  // packets are delayed. This is more of a protection mechanism. We
  // saw a case where upstream switches were not able to respond to ARP
  // in time. The underlying cause was that the upstream switch was getting
  // flooded with control plane traffic while ARP traffic was set to goto a low
  // priority queue. This lead to ARP responses to be delayed by upto 50
  // seconds. We have since fixed the issues, but it was decided to add an extra
  // safety measure here to avoid catastrophic failure if such a situation
  // arises again.On vendor devices we set ARP expiry to be as high as 1500
  // seconds.
  16: i32 maxNeighborProbes = 300
  17: i32 staleEntryInterval = 10
  18: list<AggregatePort> aggregatePorts = []
  // What admin distance to use for each potential clientID
  // These mappings map a StdClientIds to a AdminDistance
  // Predefined values for these can be found at
  // fboss/agent/if/ctrl.thrift
  19: map<i32, i32> clientIdToAdminDistance = {
        0: 20,
        1: 1,
      }
  /* Override source IP for DHCP relay packet to the DHCP server */
  20: optional string dhcpRelaySrcOverrideV4
  21: optional string dhcpRelaySrcOverrideV6
  /*
   * Override source IP for DHCP reply packet to client host.
   * This IP has to match one of interface IPs, as it is used to determines
   * which interface/VLAN the client host MAC is searched against in order to
   * send the reply to.
   */
  22: optional string dhcpReplySrcOverrideV4
  23: optional string dhcpReplySrcOverrideV6
  24: optional TrafficPolicyConfig globalEgressTrafficPolicy
  25: optional string config_version
  26: list<SflowCollector> sFlowCollectors = []
}
