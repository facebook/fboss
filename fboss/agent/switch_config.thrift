#
# Copyright 2004-present Facebook. All Rights Reserved.
#
namespace py neteng.fboss.switch_config
namespace py3 neteng.fboss
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
  FOURHUNDREDG = 400000;        // 400G
}

/**
 * The pause setting for a port
 */
struct PortPause {
  1: bool tx = false
  2: bool rx = false
}

/**
 * Using this as default is slightly better for generated bindings w/ nullable
 * languages, most notably python. Setting this as default ensures that the
 * attribute is non-null, but defaults both members to false.
*/
const PortPause NO_PAUSE = {
  "tx": false,
  "rx": false,
}

enum PortFEC {
  ON = 1,
  OFF = 2,
}

/**
 *  A Range for L4 port range checker
 *  Define a range bewteen [min, max]
 */
struct L4PortRange {
  1: i32 min
  2: i32 max
  3: bool invert = false
}

/**
 *  A Range for packet length
 *  Define a packet length range [min, max]
 */
struct PktLenRange {
  1: i16 min
  2: i16 max
  3: bool invert = false
}

enum IpType {
  ANY = 0,
  IP = 1,
  IP4 = 2,
  IP6 = 3,
}

struct Ttl {
  1: i16 value
  2: i16 mask = 0xFF
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

union MirrorEgressPort {
  1: string name
  2: i32 logicalID
}

/*
 * Mirror destination
 * At least one of the below paramaters must be set
 *
 *  egressPort - name or ID of port from which mirror traffic egresses
 *               required for SPAN
 *               optional for ERSPAN port mirroring
 *  ip - mirror to remote (ipv4) destination
 *               required and used only for ERSPAN
 *
 */
struct MirrorDestination {
  1: optional MirrorEgressPort egressPort
  2: optional string ip
}

/*
 * Mirror,
 *   - name, a logical identifier of mirror
 *   - destination, to deliver mirrored traffic
 *   - direction of a traffic to mirror
 *
 * Mirroring allows replication of data plane traffic to another destination
 * This destination can be specified either as an egress port, an egress port
 * and a remote routable ip (v4) address, or only remote routable ip (v4)
 * address.
 *
 * If only egress port is specified, then mirrored data-plane traffic egresses
 * the specified port.
 *
 * If egress port & remote ip are specified, then mirrored data-plane traffic
 * egresses the specified port and is delivered to destination ip address.
 *
 * If only remote ip is specified, then mirrored data-plane traffic is delivered
 * to the destination ip address via any possible egress port.
 *
 * traffic delivered to remote ip address is ERSPAN (type 1) encapsulated.
 * this is basically a "gre" tunnel with protocol type 0x88be
 * L2 frame is encapsulated in gre tunnel and delivered to remote routable
 * destination.
 *
 * A traffic to mirror, can be selected using twowways
 *    1) a traffic ingressing or egressing a particular port
 *    2) a traffic ingressing or egressing that meets "acl" criteria,
 *    (this mirrors a traffic ingressing/egressing switch's acl engine)
 *
 *  mirror direction if INGRESS, only ingressing traffic is mirrored
 *  mirror direction if EGRESS, only egressing traffic is mirrored
 *  mirror direction if BOTH, both ingress/egress traffic is mirrored
 *
 * Only four mirror destinations are possible
 *
 * Mirroring should be used with caution and in circumstances to debug traffic
 * issues, as this feature duplicates traffic and so doubles traffic load.
 */
struct Mirror {
  1: string name
  2: MirrorDestination destination
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
   * Valid value should between 0-255.
   * Note: Should use uint8, but `byte` in thrift is int8, so use i16 instead.
   * Otherwise we will have to use [-127,-1] to represent [128,255], which is
   * not straight-forward at all.
   */
  7: optional i16 proto

  /**
   * TCP flags(aka Control bits) Bit Map. e.g, 16 for ACK set
   * Starting from the FIN flag which is bit 0 and bit 7 is the CWR bit
   * Valid value should between 0-255.
   */
  8: optional i16 tcpFlagsBitMap

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
   * Valid value should between 0-255.
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

  19: optional string dstMac

  /* Match TTL if IPv4 and HopLimit if IPv6 */
  20: optional Ttl ttl

  /* IP type (IPv4, IPv6, ARP, MPLS... */
  21: optional IpType ipType

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

struct PacketCounterMatchAction {
  1: string counterName
}

struct SetDscpMatchAction {
  1: byte dscpValue;
}

struct MatchAction {
  1: optional QueueMatchAction sendToQueue
  2: optional PacketCounterMatchAction packetCounter_DEPRECATED
  3: optional SetDscpMatchAction setDscp
  4: optional string ingressMirror
  5: optional string egressMirror
  6: optional string counter
}

struct MatchToAction {
  // This references an ACL added to the acls list in the SwitchConfig object
  1: string matcher
  2: MatchAction action
}

enum MMUScalingFactor {
  ONE = 0
  EIGHT = 1
  ONE_128TH = 2
  ONE_64TH = 3
  ONE_32TH = 4
  ONE_16TH = 5
  ONE_8TH = 6
  ONE_QUARTER = 7
  ONE_HALF = 8
  TWO = 9
  FOUR = 10
}

// This determines how packets are scheduled on a per queue basis
enum QueueScheduling {
  // The round robin runs on all queues set to use WRR for a port
  // Required to set a weight when using this
  WEIGHTED_ROUND_ROBIN = 0
  // All packets in this type of queue are dequeued before moving on
  // to the next (lower priority) queue
  // This means this can cause starvation on other queues if not
  // configured properly
  STRICT_PRIORITY = 1
}

// Detection based on average queue length in bytes with two thresholds.
// If the queue length is below the minimum threshold, then never consider the
// queue congested. If the queue length is above the maximum threshold, then
// always consider the queue congested. If the queue length is in between the
// two thresholds, then probabilistically consider the queue congested.
// The probability grows linearly between the two thresholds. That is,
// if we call the minimum threshold m and the maximum threshold M then the
// formula for the probability at queue length m+k is:
// P(m+k) = k/(M-m) for k between 0 and M-m
struct LinearQueueCongestionDetection {
  1: required i32 minimumLength
  2: required i32 maximumLength
}

// Determines when we will consider a queue to be experiencing congestion
// for the purposes of Active Queue Management
union QueueCongestionDetection {
  1: LinearQueueCongestionDetection linear;
}

// Two behaviors are supported for Active Queue Management:
// drops and ECN marking. ECN marking is also conditional on the entity
// entering the queue being an IP packet marked as ECN-enabled. Thus, the
// two booleans can configure the following behavior during congestion:
// earlyDrop false; ecn false: Tail drops for all packets
// earlyDrop true; ecn false: Early drops for all packets
// earlyDrop false; ecn true: Tail drops for ECN-disabled, ECN for ECN-enabled
// earlyDrop true; ecn true: Early drops for ECN-disabled, ECN for ECN-enabled
enum QueueCongestionBehavior {
  EARLY_DROP = 0 // Drop packets before congested queues are totally full using
                 // an algorithm like WRED
  ECN        = 1 // Mark congestion enabled packets with Congestion Experienced
}

// Configuration for Active Queue Management of a PortQueue.
// Follows the principles outlined in RFC 7567.
struct ActiveQueueManagement {
  // How we answer the question "Is the queue congested?"
  1: required QueueCongestionDetection detection
  // How we handle packets on queues experiencing congestion
  2: required QueueCongestionBehavior behavior
}

// It is only necessary to define PortQueues for those that you want to
// change settings on
// It is only necessary to define PortQueues for those where you do not want to
// use the default settings.
// Any queues not described by config will use the system defaults of weighted
// round robin with a default weight of 1
struct PortQueue {
  1: required i16 id
  // We only use unicast in Fabric
  2: required StreamType streamType = StreamType.UNICAST
  // This value is ignored if STRICT_PRIORITY is chosen
  3: optional i32 weight
  4: optional i32 reservedBytes
  5: optional MMUScalingFactor scalingFactor
  6: required QueueScheduling scheduling
  7: optional string name
  // 8: optional i32 length (deprecated)
  9: optional i32 packetsPerSec
  10: optional i32 sharedBytes
  // Only Unicast queue supports aqms
  11: optional list<ActiveQueueManagement> aqms;
}

struct QosRule {
  1: i16 queueId
  2: list<i16> dscp = []
}

struct QosPolicy {
  1: string name
  2: list<QosRule> rules
}

struct TrafficPolicyConfig {
  // Order of entries determines priority of acls when applied
  1: list<MatchToAction> matchToAction = []
  2: optional string defaultQosPolicy
  3: optional map<i32, string> portIdToQosPolicy
}

struct CPUTrafficPolicyConfig {
  1: optional TrafficPolicyConfig trafficPolicy
  2: optional map<PacketRxReason, i16> rxReasonToCPUQueue
}

enum PacketRxReason {
  UNMATCHED    = 0 // all packets for which there's not a explicit match rule
  ARP          = 1
  DHCP         = 2
  BPDU         = 3
  L3_SLOW_PATH = 4 // L3 slow path processed pkt
  L3_DEST_MISS = 5 // L3 DIP miss
  TTL_1        = 6 // L3UC or IPMC packet with TTL equal to 1
  CPU_IS_NHOP  = 7 // The CPU port is the next-hop in the routing table
}

enum PortLoopbackMode {
  NONE  = 0
  PHY   = 1
  MAC   = 2
}

enum LLDPTag {
  PDU_END     = 0,
  CHASSIS     = 1,
  PORT        = 2,
  TTL         = 3,
  PORT_DESC   = 4,
  SYSTEM_NAME = 5,
  SYSTEM_DESC = 6,
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
  8: PortSpeed speed = DEFAULT

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
   * There are multiple queues per port
   * This allows defining their attributes
   */
  12: list<PortQueue> queues = []

  /**
   * pause configuration
   */
  13: PortPause pause = NO_PAUSE

  /**
   * sFlow sampling rate for ingressing packets.
   * Every 1/sFlowIngressRate ingressing packets will be sampled.
   * 0 indicates no sampling while 1 indicates sampling all packets.
   */
  14: i64 sFlowIngressRate = 0

  /**
   * sFlow sampling rate for egressing packets.
   * Every 1/sFlowEgressRate egressing packets will be sampled.
   * 0 indicates no sampling while 1 indicates sampling all packets.
   */
  15: i64 sFlowEgressRate = 0

  /**
   * Should FEC be on for this port?
   */
  16: PortFEC fec = PortFEC.OFF

  /*
   * Setup port in loopback mode
   */
   17: PortLoopbackMode loopbackMode = PortLoopbackMode.NONE
  /**
   * if port is mirrored?
   */
  18: optional string ingressMirror
  19: optional string egressMirror

  /**
   * The LLDP values we expect to receive on this port, to validate
   * against the actual values received from the neighbour.
   *
   * Only LLDP tags with a string representation, or a clear transform to
   * a string (e.g. uint16_t) can be specified.
   *
   * For composite tags, such as Port/Chassis ID, only the string portion
   * is compared.
   */
  20: map<LLDPTag, string> expectedLLDPValues
}

enum LacpPortRate {
  SLOW = 0,
  FAST = 1,
}

enum LacpPortActivity {
  PASSIVE = 0,
  ACTIVE = 1,
}

struct AggregatePortMember {
  /**
   * Member ports are identified according to their logicalID, as defined in
   * struct Port.
   */
  1: i32 memberPortID
  2: i32 priority
  3: LacpPortRate rate = FAST
  4: LacpPortActivity activity = ACTIVE
}

union MinimumCapacity {
  1: byte linkCount,
  2: double linkPercentage,
}

const MinimumCapacity ALL_LINKS = {
  "linkPercentage": 1
}

struct AggregatePort {
  1: i16 key
  2: string name
  3: string description
  4: list<AggregatePortMember> memberPorts
  5: MinimumCapacity minimumCapacity = ALL_LINKS
}

struct Lacp {
  1: string systemID    // = cpuMAC
  2: i32 systemPriority // = (2 ^ 16) - 1
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
  10: bool isStateSyncDisabled = 0
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

enum LoadBalancerID {
  ECMP = 1,
  AGGREGATE_PORT = 2,
}

enum IPv4Field {
  SOURCE_ADDRESS      = 1,
  DESTINATION_ADDRESS = 2
}

enum IPv6Field {
  SOURCE_ADDRESS      = 1,
  DESTINATION_ADDRESS = 2,
  FLOW_LABEL          = 3
}

enum TransportField {
  SOURCE_PORT      = 1,
  DESTINATION_PORT = 2
}

struct Fields {
  1: set<IPv4Field> ipv4Fields
  2: set<IPv6Field> ipv6Fields
  3: set<TransportField> transportFields
}

enum HashingAlgorithm {
  CRC16_CCITT = 1,

  CRC32_LO = 3,
  CRC32_HI = 4,

  CRC32_ETHERNET_LO = 5,
  CRC32_ETHERNET_HI = 6,

  CRC32_KOOPMAN_LO  = 7,
  CRC32_KOOPMAN_HI  = 8,
}

struct LoadBalancer {
  1: LoadBalancerID id
  2: Fields fieldSelection
  3: HashingAlgorithm algorithm
  // If not set, the seed will be chosen so as to remain constant across process
  // restarts
  4: optional i32 seed
}

enum CounterType {
  PACKETS = 0,
  BYTES = 1,
}

struct TrafficCounter {
  1: string name
  2: list<CounterType> types = [PACKETS]
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
  12: list<StaticRouteWithNextHops> staticRoutesWithNhops = [];
  // Prefixes for which to drop traffic
  13: list<StaticRouteNoNextHops> staticRoutesToNull = [];
  // Prefixes for which to send traffic to CPU
  14: list<StaticRouteNoNextHops> staticRoutesToCPU = [];
  // List of all ACLs that are available for use by various agent components
  // ACLs declared here can be referenced in other places in order to tie
  // actions to them, e.g. as part of a MatchToAction
  // Only DROP acls are applied directly, all others require being referenced
  // elsewhere in order to be meaningful
  // Ordering of DROP acls define their priority
  15: list<AclEntry> acls = []
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
        0: 20,    // BGP
        786: 10,  // OPENR
        1: 1,     // Static routes from config
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
  24: optional TrafficPolicyConfig globalEgressTrafficPolicy_DEPRECATED
  25: optional string config_version
  26: list<SflowCollector> sFlowCollectors = []
  27: optional Lacp lacp
  28: list<PortQueue> cpuQueues = []
  29: optional CPUTrafficPolicyConfig cpuTrafficPolicy
  30: list<LoadBalancer> loadBalancers = []
  31: optional TrafficPolicyConfig dataPlaneTrafficPolicy
  32: list<Mirror> mirrors = []
  33: list<TrafficCounter> trafficCounters = []
  34: list<QosPolicy> qosPolicies = []
}
