#
# Copyright 2004-present Facebook. All Rights Reserved.
#
namespace py neteng.fboss.switch_config
namespace cpp facebook.fboss.cfg

/**
 * Port state
 */
enum PortState {
  DOWN = 0,  // Down, transmits an idle pattern
  POWER_DOWN = 1,  // Down, no signal transmitted
  UP = 2,
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
 * The action for an access control entry
 */
enum AclAction {
  DENY = 0,
  PERMIT = 1,
}

/**
 * Configuration for a single logical port
 */
struct Port {
  1: i32 logicalID
  /*
   * Whether the port is enabled.  Set this to UP for normal operation.
   */
  2: PortState state
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

/**
 * An access control entry
 */
struct AclEntry {
  /**
   * Unique identifier of an AclEntry. Entries with smaller ID will have
   * higher priority (matched first). Please make sure all AclEntry has a unique
   * ID so that they can be totally ordered.
   */
  1: i32 id

  /**
   * Actions to take
   */
  2: AclAction action

  /**
   * IP addresses with mask. e.g. 192.168.0.0/16. Can be either V4 or V6
   */
  3: optional string srcIp
  4: optional string dstIp

  /**
   * L4 ports (TCP/UDP). Note that this is NOT the switch port.
   */
  5: optional i16 l4SrcPort
  6: optional i16 l4DstPort

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
  // The order of AclEntry does _not_ determine its priority.
  // Highest priority entry comes with smallest ID.
  15: optional list<AclEntry> acls = []
  // Set max number of probes to a sufficiently high value
  // to allow for the case where we are probing and the
  // agent on other end is restarting.
  16: i32 maxNeighborProbes = 30
  17: i32 staleEntryInterval = 10
}
