namespace cpp2 facebook.fboss
namespace go neteng.fboss.ctrl
namespace php fboss
namespace py neteng.fboss.ctrl
namespace py3 neteng.fboss
namespace py.asyncio neteng.fboss.asyncio.ctrl
namespace rust facebook.fboss

include "fboss/agent/if/fboss.thrift"
include "common/fb303/if/fb303.thrift"
include "common/network/if/Address.thrift"
include "fboss/agent/if/mpls.thrift"
include "fboss/agent/if/optic.thrift"
include "fboss/qsfp_service/if/transceiver.thrift"
include "fboss/agent/platform_config.thrift"

typedef binary (cpp2.type = "::folly::fbstring") fbbinary
typedef string (cpp2.type = "::folly::fbstring") fbstring

const i32 DEFAULT_CTRL_PORT = 5909

// Using the defaults from here:
// https://en.wikipedia.org/wiki/Administrative_distance
enum AdminDistance {
  DIRECTLY_CONNECTED = 0,
  STATIC_ROUTE = 1,
  OPENR = 10,
  EBGP = 20,
  IBGP = 200,
  MAX_ADMIN_DISTANCE = 255
}

// SwSwitch run states. SwSwitch moves forward from a
// lower numbered state to the next
enum SwitchRunState {
  UNINITIALIZED = 0,
  INITIALIZED = 1,
  CONFIGURED = 2,
  FIB_SYNCED = 3,
  EXITING = 4
}

enum SSLType {
  DISABLED = 0,
  PERMITTED = 1,
  REQUIRED = 2,
}

enum PortLedExternalState {
  NONE = 0,
  CABLING_ERROR = 1,
  EXTERNAL_FORCE_ON = 2,
  EXTERNAL_FORCE_OFF = 3,
}

struct IpPrefix {
  1: required Address.BinaryAddress ip,
  2: required i16 prefixLength,
}

struct NextHopThrift {
  1: Address.BinaryAddress address,
  // Default weight of 0 represents an ECMP route.
  // This default is chosen for two reasons:
  // 1) We rely on the arithmetic properties of 0 for ECMP vs UCMP route
  //    resolution calculations. A 0 weight next hop being present at a variety
  //    of layers in a route resolution tree will cause the entire route
  //    resolution to use ECMP.
  // 2) A client which does not set a value will result in
  //    0 being populated even with strange behavior in the client language
  //    which is consistent with C++
  2: i32 weight = 0,
  // MPLS encapsulation information for IP->MPLS and MPLS routes
  3: optional mpls.MplsAction mplsAction,
}

struct UnicastRoute {
  1: required IpPrefix dest,
  // NOTE: nextHopAddrs was once required. While we work on
  // fully deprecating it, we need to be extra careful and
  // ensure we don't crash clients/servers that still see it as required.
  2: list<Address.BinaryAddress> nextHopAddrs,
  3: optional AdminDistance adminDistance,
  4: list<NextHopThrift> nextHops,
}

struct MplsRoute {
  1: required mpls.MplsLabel topLabel,
  3: optional AdminDistance adminDistance,
  4: list<NextHopThrift> nextHops,
}

struct ClientAndNextHops {
  1: required i32 clientId,
  // Deprecated in favor of '3: nextHops'
  2: required list<Address.BinaryAddress> nextHopAddrs,
  3: required list<NextHopThrift> nextHops,
}

struct IfAndIP {
  1: required i32 interfaceID,
  2: required Address.BinaryAddress ip,
}

struct RouteDetails {
  1: required IpPrefix dest,
  2: required string action,
  // Deprecated in favor of '7: nextHops'
  3: required list<IfAndIP> fwdInfo,
  4: required list<ClientAndNextHops> nextHopMulti,
  5: required bool isConnected,
  6: optional AdminDistance adminDistance,
  7: list<NextHopThrift> nextHops,
}

struct MplsRouteDetails {
  1: mpls.MplsLabel topLabel
  2: string action
  3: list<ClientAndNextHops> nextHopMulti,
  4: AdminDistance adminDistance,
  5: list<NextHopThrift> nextHops,
}

struct ArpEntryThrift {
  1: string mac,
  2: i32 port,
  3: string vlanName,
  4: Address.BinaryAddress ip,
  5: i32 vlanID,
  6: string state,
  7: i32 ttl,
  8: i32 classID,
}

enum L2EntryType {
  L2_ENTRY_TYPE_PENDING = 0,
  L2_ENTRY_TYPE_VALIDATED = 1,
}

struct L2EntryThrift {
  1: string mac,
  2: i32 port,
  3: i32 vlanID,
  // Add information about l2 entries associated with
  // trunk ports. Only one of port, trunk is valid. If
  // trunk is set we look at that.
  4: optional i32 trunk
  5: L2EntryType l2EntryType
  6: optional i32 classID,
}

enum LacpPortRateThrift {
  SLOW = 0,
  FAST = 1,
}

enum LacpPortActivityThrift {
  PASSIVE = 0,
  ACTIVE = 1,
}

struct AggregatePortMemberThrift {
  1: i32 memberPortID
  2: bool isForwarding
  3: i32 priority
  4: LacpPortRateThrift rate
  5: LacpPortActivityThrift activity
}

struct AggregatePortThrift {
  1: i32 key
  2: list<AggregatePortMemberThrift> memberPorts
  3: string name
  4: string description
  5: i32 systemPriority
  6: string systemID
  7: byte minimumLinkCount
  8: bool isUp
}

struct LacpStateThrift {
  1: bool active      ,
  2: bool shortTimeout,
  3: bool aggregatable,
  4: bool inSync      ,
  5: bool collecting  ,
  6: bool distributing,
  7: bool defaulted   ,
  8: bool expired     ,
}

struct LacpEndpoint {
  1: i32 systemPriority,
  2: string systemID,
  3: i32 key,
  4: i32 portPriority,
  5: i32 port,
  6: LacpStateThrift state,
}

struct LacpPartnerPair {
  1: LacpEndpoint localEndpoint,
  2: LacpEndpoint remoteEndpoint,
}

struct InterfaceDetail {
  1: string interfaceName,
  2: i32 interfaceId,
  3: i32 vlanId,
  4: i32 routerId,
  5: string mac,
  6: list<IpPrefix> address,
  7: i32 mtu,
}

struct ProductInfo {
  1: string oem,
  2: string product,
  3: string serial,
  4: string macRangeStart,
  5: i16 macRangeSize,
  6: string mfgDate,
  7: string systemPartNumber,
  8: string assembledAt,
  9: string pcbManufacturer,
  10: string assetTag,
  11: string partNumber,
  12: string odmPcbaPartNumber,
  13: string odmPcbaSerial,
  14: string fbPcbPartNumber,
  15: i16 version,
  16: i16 subVersion,
  17: i16 productionState,
  18: i16 productVersion,
  19: string bmcMac,
  20: string mgmtMac,
  21: string fabricLocation,
  22: string fbPcbaPartNumber,
}

/*
 * Values in these counters are cumulative since the last time the agent
 * started.
 */
struct PortErrors {
  1: i64 errors,
  2: i64 discards,
}

struct QueueStats {
  1: i64 congestionDiscards,
  2: i64 outBytes,
}

/*
 * Values in these counters are cumulative since the last time the agent
 * started.
 */
struct PortCounters {
  // avoid typechecker error here as bytes is a py3 reserved keyword
  1: i64 bytes (py3.name = "bytes_"),
  2: i64 ucastPkts,
  3: i64 multicastPkts,
  4: i64 broadcastPkts,
  5: PortErrors errors,
  6: list<QueueStats> unicast = [];
}

enum PortAdminState {
  DISABLED = 0,
  ENABLED = 1,
}

enum PortOperState {
  DOWN = 0,
  UP = 1,
}

struct LinearQueueCongestionDetection {
  1: i32 minimumLength
  2: i32 maximumLength
}

struct QueueCongestionDetection {
  1: optional LinearQueueCongestionDetection linear
}

enum QueueCongestionBehavior {
  EARLY_DROP = 0,
  ECN = 1,
}

struct ActiveQueueManagement {
  1: QueueCongestionDetection detection
  2: QueueCongestionBehavior behavior
}

struct Range {
  1: i32 minimum
  2: i32 maximum
}

union PortQueueRate {
  1: Range pktsPerSec
  2: Range kbitsPerSec
}

struct PortQueueThrift {
  1: i32 id,
  2: string name = "",
  3: string mode,
  4: optional i32 weight,
  5: optional i32 reservedBytes,
  6: optional string scalingFactor,
  7: optional list<ActiveQueueManagement> aqms,
  8: optional PortQueueRate portQueueRate,
  9: optional i32 bandwidthBurstMinKbits,
  10: optional i32 bandwidthBurstMaxKbits,
  11: optional list<byte> dscps,
}

struct PortInfoThrift {
  1: i32 portId,
  2: i64 speedMbps,
  3: PortAdminState adminState,
  4: PortOperState operState,
  5: list<i32> vlans,

  10: PortCounters output,
  11: PortCounters input,
  12: string name,
  13: string description,
  // TODO(pgardideh): remove this in favor of fecMode
  14: bool fecEnabled, // Forward Error Correction port setting
  15: bool txPause = false,
  16: bool rxPause = false,
  17: list<PortQueueThrift> portQueues = [],
  18: string fecMode,
  19: string profileID,
}

struct NdpEntryThrift {
  1: Address.BinaryAddress ip,
  2: string mac,
  3: i32 port,
  4: string vlanName,
  5: i32 vlanID,
  6: string state,
  7: i32 ttl,
  8: i32 classID,
}

enum BootType {
  UNINITIALIZED = 0,
  COLD_BOOT = 1,
  WARM_BOOT = 2
}

struct TransceiverIdxThrift {
  1: i32 transceiverId,
  2: optional i32 channelId,  # deprecated
  3: optional list<i32> channels,
}

struct PortStatus {
  1: bool enabled,
  2: bool up,
  3: optional bool present,  # deprecated
  4: optional TransceiverIdxThrift transceiverIdx,
  5: i64 speedMbps,  // TODO: i32 (someone is optimistic about port speeds)
  6: string profileID,
}

enum PrbsComponent {
  ASIC = 0,
  GB_SYSTEM = 1,
  GB_LINE = 2
}

struct PrbsState {
  1: bool enabled = false,
  2: i32 polynominal
}

struct PrbsLaneStats {
  1: i32 laneId,
  2: bool locked,
  3: double ber,
  4: double maxBer,
  5: i32 numLossOfLock,
  6: i32 timeSinceLastLocked,
  7: i32 timeSinceLastClear,
}

struct PrbsStats {
  1: i32 portId,
  2: PrbsComponent component,
  3: list<PrbsLaneStats> laneStats,
}

enum CaptureDirection {
  CAPTURE_ONLY_RX = 0,
  CAPTURE_ONLY_TX = 1,
  CAPTURE_TX_RX = 2
}


enum CpuCosQueueId {
  LOPRI = 0,
  DEFAULT = 1,
  MIDPRI = 2,
  HIPRI = 9
}

struct RxCaptureFilter {
  1: list<CpuCosQueueId> cosQueues
  # can put additional Rx filters here if need be
}

struct CaptureFilter {
  1: RxCaptureFilter rxCaptureFilter;
}

struct CaptureInfo {
  // A name identifying the packet capture
  1: string name
  /*
   * Stop capturing after the specified number of packets.
   *
   * This parameter is required to make sure the capture will stop.
   * In case your filter matches more packets than you expect, we don't want
   * to consume lots of space and other resources by capturing an extremely
   * large number of packets.
   */
  2: i32 maxPackets
  3: CaptureDirection direction = CAPTURE_TX_RX
  /*
   * set of criteria that packet must meet to be captured
   */
  4: CaptureFilter  filter
}

struct RouteUpdateLoggingInfo {
  // The prefix to log route updates for
  1: IpPrefix prefix
  /*
   * A name to split up requests for logging. Allows two different clients
   * to separately request starting/stopping logging for the same prefixes
   * without affecting each other.
   */
  2: string identifier
  // Should we log an update to a route to a more specific prefix than "prefix"
  3: bool exact
}

struct MplsRouteUpdateLoggingInfo {
  // The label to log route updates for label, -1 for all labels
  1: mpls.MplsLabel label
  /*
   * A name to split up requests for logging. Allows two different clients
   * to separately request starting/stopping logging for the same prefixes
   * without affecting each other.
   */
  2: string identifier
}

/*
 * Information about an LLDP neighbor
 */
struct LinkNeighborThrift {
  1: i32 localPort
  2: i32 localVlan
  3: i32 chassisIdType
  4: i32 portIdType
  5: i32 originalTTL
  6: i32 ttlSecondsLeft
  7: string srcMac
  8: binary chassisId
  9: string printableChassisId
  10: binary portId
  11: string printablePortId
  12: optional string systemName
  13: optional string systemDescription
  14: optional string portDescription
  15: optional string localPortName
}

enum ClientID {
  BGPD = 0,
  STATIC_ROUTE = 1,
  INTERFACE_ROUTE = 2,
  LINKLOCAL_ROUTE = 3,
  STATIC_INTERNAL = 700,
  OPENR = 786,
}

struct AclEntryThrift {
  1: i32 priority
  2: string name
  3: Address.BinaryAddress srcIp
  4: i32 srcIpPrefixLength
  5: Address.BinaryAddress dstIp
  6: i32 dstIpPrefixLength
  7: optional byte proto
  8: optional byte tcpFlagsBitMap
  9: optional i16 srcPort
  10: optional i16 dstPort
  11: optional byte ipFrag
  12: optional byte icmpType
  13: optional byte icmpCode
  14: optional byte dscp
  15: optional byte ipType
  16: optional i16 ttl
  17: optional string dstMac
  18: optional i16 l4SrcPort
  19: optional i16 l4DstPort
  20: optional byte lookupClass
  21: string actionType
  22: optional byte lookupClassL2
}

struct ClientInformation {
  1: optional fbstring username,
  2: optional fbstring hostname,
}

enum HwObjectType {
  PORT = 0,
  LAG = 1,
  VIRTUAL_ROUTER = 2,
  NEXT_HOP = 3,
  NEXT_HOP_GROUP = 4,
  ROUTER_INTERFACE = 5,
  CPU_TRAP = 6,
  HASH = 7,
  MIRROR = 8,
  QOS_MAP = 9,
  QUEUE = 10,
  SCHEDULER = 11,
  L2_ENTRY = 12,
  NEIGHBOR_ENTRY = 13,
  ROUTE_ENTRY = 14,
  VLAN = 15,
  BRIDGE = 16,
  BUFFER = 17,
  ACL_ = 18,
  DEBUG_COUNTER = 19,
  TELEMETRY = 20,
}

service FbossCtrl extends fb303.FacebookService {
  /*
   * Retrieve up-to-date counters from the hardware, and publish all
   * thread-local stats to the global state.  This way the next getCounters()
   * call will receive up-to-date stats.
   *
   * Without this call, the stats may be delayed by up to 1 second.  In
   * general, we expect most users to be okay with the 1 second delay.
   * However, some tests use this API to check for up-to-date statistics
   * immediately after an event has occurred.
   */
  void flushCountersNow()
    throws (1: fboss.FbossBaseError error)

  /*
   * Add/Delete IPv4/IPV6 routes
   * - decide if it is v4 or v6 from destination ip address
   * - using clientID to identify who is adding routes, BGP or static
   */
  void addUnicastRoute(1: i16 clientId, 2: UnicastRoute r)
    throws (1: fboss.FbossBaseError error)
  void deleteUnicastRoute(1: i16 clientId, 2: IpPrefix r)
    throws (1: fboss.FbossBaseError error)
  void addUnicastRoutes(1: i16 clientId, 2: list<UnicastRoute> r)
    throws (1: fboss.FbossBaseError error)
  void deleteUnicastRoutes(1: i16 clientId, 2: list<IpPrefix> r)
    throws (1: fboss.FbossBaseError error)
  void syncFib(1: i16 clientId, 2: list<UnicastRoute> routes)
    throws (1: fboss.FbossBaseError error)

  void addUnicastRouteInVrf(1: i16 clientId, 2: UnicastRoute r, 3: i32 vrf)
    throws (1: fboss.FbossBaseError error)
  void deleteUnicastRouteInVrf(1: i16 clientId, 2: IpPrefix r, 3: i32 vrf)
    throws (1: fboss.FbossBaseError error)
  void addUnicastRoutesInVrf(
    1: i16 clientId,
    2: list<UnicastRoute> r,
    3: i32 vrf
  ) throws (1: fboss.FbossBaseError error)
  void deleteUnicastRoutesInVrf(
    1: i16 clientId,
    2: list<IpPrefix> r,
    3: i32 vrf
  ) throws (1: fboss.FbossBaseError error)
  void syncFibInVrf(1: i16 clientId, 2: list<UnicastRoute> routes, 3: i32 vrf)
    throws (1: fboss.FbossBaseError error)

  /*
   * Send packets in binary or hex format to controller.
   *
   * This injects the packet into the controller, as if it had been received
   * from a front-panel port.
   */
  void sendPkt(1: i32 port, 2: i32 vlan, 3: fbbinary data)
    throws (1: fboss.FbossBaseError error)
  void sendPktHex(1: i32 port, 2: i32 vlan, 3: fbstring hex)
    throws (1: fboss.FbossBaseError error)

  /*
   * Transmit a packet out a specific front panel port.
   *
   * The data should contain the full ethernet frame (not including the final
   * frame check sequence).  It must have an explicit VLAN tag indicating what
   * VLAN to send the packet on.  (The VLAN tag may be stripped out of the
   * packet actually sent by the hardware, based on the hardware VLAN tagging
   * configuration for this VLAN+port.)
   */
  void txPkt(1: i32 port, 2: fbbinary data)
    throws (1: fboss.FbossBaseError error)

  /*
   * Transmit a packet out to a specific VLAN.
   *
   * This causes the packet to be sent out the front panel ports that belong to
   * this VLAN.
   *
   * The data should contain the full ethernet frame (not including the final
   * frame check sequence).  It must have an explicit VLAN tag indicating what
   * VLAN to send the packet on.  (The VLAN tag may be stripped out of the
   * packet actually sent by the hardware, based on the hardware VLAN tagging
   * configuration for this VLAN.)
   */
  void txPktL2(1: fbbinary data) throws (1: fboss.FbossBaseError error)

  /*
   * Transmit an L3 packet.
   *
   * The payload should consist only of the layer 3 header and payload.
   * The controller will add an appropriate ethernet frame header.  It will
   * contain the correct next hop information based on the layer 3 header.
   */
  void txPktL3(1: fbbinary payload) throws (1: fboss.FbossBaseError error)

  /*
   * Flush the ARP/NDP entry with the specified IP address.
   *
   * If the vlanId is 0, then this command will iterate through all VLANs
   * and flush all entries it finds with this IP.
   *
   * Returns the number of entries flushed.
   */
  i32 flushNeighborEntry(1: Address.BinaryAddress ip, 2: i32 vlanId)

  /*
   * Inband addresses
   */
  list<Address.Address> getVlanAddresses(1: i32 vlanId)
    throws (1: fboss.FbossBaseError error)
  list<Address.Address> getVlanAddressesByName(1: string vlan)
    throws (1: fboss.FbossBaseError error)
  list<Address.BinaryAddress> getVlanBinaryAddresses(1: i32 vlanId)
    throws (1: fboss.FbossBaseError error)
  list<Address.BinaryAddress> getVlanBinaryAddressesByName(1: string vlan)
    throws (1: fboss.FbossBaseError error)
  /*
   * Returns a list of IP route as per the route table for the
   * given address
   *
   * TODO (allwync): get rid of getIpRoute after agent code with thrift
   * implementation of getIpRouteDetails is pushed everywhere
   */
  UnicastRoute getIpRoute(1: Address.Address addr 2: i32 vrfId)
    throws (1: fboss.FbossBaseError error)
  RouteDetails getIpRouteDetails(1: Address.Address addr 2: i32 vrfId)
    throws (1: fboss.FbossBaseError error)
  map<i32, InterfaceDetail> getAllInterfaces()
    throws (1: fboss.FbossBaseError error)
  void registerForNeighborChanged()
    throws (1: fboss.FbossBaseError error) (thread='eb')
  list<string> getInterfaceList()
    throws (1: fboss.FbossBaseError error)
  /*
   * TODO (allwync): get rid of getRouteTable after agent code with thrift
   * implementation of getRouteTableDetails is pushed everywhere
   */
  list<UnicastRoute> getRouteTable()
    throws (1: fboss.FbossBaseError error)
  list<UnicastRoute> getRouteTableByClient(1: i16 clientId)
    throws (1: fboss.FbossBaseError error)
  list<RouteDetails> getRouteTableDetails()
    throws (1: fboss.FbossBaseError error)
  InterfaceDetail getInterfaceDetail(1: i32 interfaceId)
    throws (1: fboss.FbossBaseError error)

  /* MPLS route API */
   void addMplsRoutes(
     1: i16 clientId,
     2: list<MplsRoute> routes,
   ) throws (1: fboss.FbossBaseError error)

   void deleteMplsRoutes(
     1: i16 clientId,
     2: list<i32> topLabels,
   ) throws (1: fboss.FbossBaseError error)

   /* Flush previous routes and install new routes without disturbing traffic.
    * Similar to syncFib API */
   void syncMplsFib(
     1: i16 clientId,
     2: list<MplsRoute> routes,
   ) throws (1: fboss.FbossBaseError error)

   /* Retrieve list of MPLS routes per client */
   list<MplsRoute> getMplsRouteTableByClient(
     1: i16 clientId
   ) throws (1: fboss.FbossBaseError error)

   /* Retrieve list of MPLS entries */
   list<MplsRouteDetails> getAllMplsRouteDetails() throws (
      1: fboss.FbossBaseError error
   )

    /* Retrieve MPLS entry for given label */
    MplsRouteDetails getMplsRouteDetails(1: mpls.MplsLabel topLabel) throws (
       1: fboss.FbossBaseError error
    )

  /*
   * Return the admin and oper state of ports in the list (all ports
   * if list is empty).
   */
  map<i32, PortStatus> getPortStatus(1: list<i32> ports)
    throws (1: fboss.FbossBaseError error)

  /*
   * Administratively enable/disable a port.  Useful to temporarily change
   * the admin state of a port, for example to simulate a link flap.  This
   * does not affect the configuraton, so the port will return to configured
   * state after, for example, restart.
   */
  void setPortState(1: i32 portId, 2: bool enable)
    throws (1: fboss.FbossBaseError error)

  /*
   * Change the PRBS setting on a port. Useful when debugging a link
   * down or flapping issue.
   */
  void setPortPrbs(
    1: i32 portId,
    2: PrbsComponent component,
    3: bool enable,
    4: i32 polynominal
  ) throws (1: fboss.FbossBaseError error)

  /*
   * Get the PRBS stats on a port. Useful when debugging a link
   * down or flapping issue.
   */
  PrbsStats getPortPrbsStats(
    1: i32 portId,
    2: PrbsComponent component,
  ) throws (1: fboss.FbossBaseError error)

  /*
   * Clear the PRBS stats counter on a port. Useful when debugging a link
   * down or flapping issue.
   * This clearPortPrbsStats will result in:
   * 1. reset ber (due to reset accumulated error count if implemented)
   * 2. reset maxBer
   * 3. reset numLossOfLock to 0
   * 4. set timeLastCleared to now
   * 5. set timeLastLocked to timeLastCollect if locked else epoch
   * 6. locked status not changed
   * 7. timeLastCollect not changed
   */
  void clearPortPrbsStats(
    1: i32 portId,
    2: PrbsComponent component,
  ) throws (1: fboss.FbossBaseError error)

  /*
   * Return info related to the port including name, description, speed,
   * counters, ...
   */
  PortInfoThrift getPortInfo(1: i32 portId)
    throws (1: fboss.FbossBaseError error)
  map<i32, PortInfoThrift> getAllPortInfo()
    throws (1: fboss.FbossBaseError error)

  /* clear stats for specified port(s) */
  void clearPortStats(1: list<i32> ports)

  /* Legacy names for getPortInfo() and getAllPortInfo() */
  PortInfoThrift getPortStats(1: i32 portId)
    throws (1: fboss.FbossBaseError error)
  map<i32, PortInfoThrift> getAllPortStats()
    throws (1: fboss.FbossBaseError error)

  /* Return running config */
  string getRunningConfig()
    throws (1: fboss.FbossBaseError error)

  list<ArpEntryThrift> getArpTable()
    throws (1: fboss.FbossBaseError error)
  list<NdpEntryThrift> getNdpTable()
    throws (1: fboss.FbossBaseError error)
  list<L2EntryThrift> getL2Table()
    throws (1: fboss.FbossBaseError error)
  list<AclEntryThrift> getAclTable()
    throws (1: fboss.FbossBaseError error)

  AggregatePortThrift getAggregatePort(1: i32 aggregatePortID)
    throws (1: fboss.FbossBaseError error)
  list<AggregatePortThrift> getAggregatePortTable()
    throws (1: fboss.FbossBaseError error)

  LacpPartnerPair getLacpPartnerPair(1: i32 portID)
    throws (1: fboss.FbossBaseError error)
  list<LacpPartnerPair> getAllLacpPartnerPairs()
    throws (1: fboss.FbossBaseError error)

  /*
   * Information about Hw state, often useful for debugging
   * on a box
   */
  string getHwDebugDump()
  /*
   * String formatted information of givens Hw Objects.
   */
  string listHwObjects(1: list<HwObjectType> objects)
    throws (1: fboss.FbossBaseError error)

  /*
   * Type of boot performed by the controller
   */
  BootType getBootType()

  /*
   * Get the list of neighbors discovered via LLDP
   */
  list<LinkNeighborThrift> getLldpNeighbors()
    throws (1: fboss.FbossBaseError error)

  /*
   * Start a packet capture
   */
  void startPktCapture(1: CaptureInfo info)
    throws (1: fboss.FbossBaseError error)
  void stopPktCapture(1: string name)
    throws (1: fboss.FbossBaseError error)
  void stopAllPktCaptures()
    throws (1: fboss.FbossBaseError error)

  /*
   * Log all updates to routes that match this prefix, or are more
   * specific.
   */
  void startLoggingRouteUpdates(1: RouteUpdateLoggingInfo info)
  void stopLoggingRouteUpdates(1: IpPrefix prefix, 2: string identifier)
  void stopLoggingAnyRouteUpdates(1: string identifier)

  list<RouteUpdateLoggingInfo> getRouteUpdateLoggingTrackedPrefixes()

  /*
   * Log all updates to mpls routes for given label
   */
  void startLoggingMplsRouteUpdates(1: MplsRouteUpdateLoggingInfo info)
  void stopLoggingMplsRouteUpdates(1: MplsRouteUpdateLoggingInfo info)
  void stopLoggingAnyMplsRouteUpdates(1: string identifier)
  list<MplsRouteUpdateLoggingInfo> getMplsRouteUpdateLoggingTrackedLabels()

  void keepalive()

  i32 getIdleTimeout()
    throws (1: fboss.FbossBaseError error)

  /*
   * Return product information
   */
  ProductInfo getProductInfo()
    throws (1: fboss.FbossBaseError error)

  /*
   * Force reload configurations from the config file. Useful when config file
   * has changed since the agent started.
   */
  void reloadConfig()

  /*
   * Serialize switch state at path pointed by JSON pointer
   */
  string getCurrentStateJSON(1: string jsonPointer)

  /*
   * Apply patch at given path within the state tree. jsonPatch must  be
   * a valid JSON object string
   */
  void patchCurrentStateJSON(1: string jsonPointer, 2: string jsonPatch)

  /*
  * Switch run state
  */
  SwitchRunState getSwitchRunState()

  SSLType getSSLPolicy()
    throws (1: fboss.FbossBaseError error)

  /*
  * Enable external control for port LED lights
  */
  void setExternalLedState(1: i32 portNum, 2: PortLedExternalState ledState)
    throws (1: fboss.FbossBaseError error)

  /*
   * Enables submitting diag cmds to the switch
   */
  fbstring diagCmd(
    1: fbstring cmd,
    2: ClientInformation client,
    3: i16 serverTimeoutMsecs = 0
  )

  /*
  * Return the system's platform mapping (see platform_config.thrift)
  */
  platform_config.PlatformMapping getPlatformMapping()
    throws (1: fboss.FbossBaseError error)
}

service NeighborListenerClient extends fb303.FacebookService {
  /*
   * Sends list of neighbors that have changed to the subscriber.
   *
   * These come in the form of ip address strings which have been added
   * since the last notification. Changes are not queued between
   * subscriptions.
   */
  void neighborsChanged(1: list<string> added, 2: list<string> removed)
    throws (1: fboss.FbossBaseError error)
}
