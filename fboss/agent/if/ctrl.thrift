namespace cpp2 facebook.fboss
namespace d neteng.fboss.ctrl
namespace go neteng.fboss.ctrl
namespace php fboss
namespace py neteng.fboss.ctrl
namespace py.asyncio neteng.fboss.asyncio.ctrl

include "fboss/agent/if/fboss.thrift"
include "common/fb303/if/fb303.thrift"
include "common/network/if/Address.thrift"
include "fboss/agent/if/optic.thrift"
include "fboss/agent/if/highres.thrift"
include "fboss/qsfp_service/if/transceiver.thrift"

typedef binary (cpp2.type = "::folly::fbstring") fbbinary
typedef string (cpp2.type = "::folly::fbstring") fbstring

const i32 DEFAULT_CTRL_PORT = 5909

// Using the defaults from here:
// https://en.wikipedia.org/wiki/Administrative_distance
enum AdminDistance {
  DIRECTLY_CONNECTED = 0,
  STATIC_ROUTE = 1,
  EBGP = 20,
  IBGP = 200,
  NETLINK_LISTENER = 225,
  MAX_ADMIN_DISTANCE = 255
}

struct IpPrefix {
  1: required Address.BinaryAddress ip,
  2: required i16 prefixLength,
}

struct UnicastRoute {
  1: required IpPrefix dest,
  2: required list<Address.BinaryAddress> nextHopAddrs,
  3: optional AdminDistance adminDistance,
}

struct ClientAndNextHops {
  1: required i32 clientId,
  2: required list<Address.BinaryAddress> nextHopAddrs,
}

struct IfAndIP {
  1: required i32 interfaceID,
  2: required Address.BinaryAddress ip,
}

struct RouteDetails {
  1: required IpPrefix dest,
  2: required string action,
  3: required list<IfAndIP> fwdInfo,
  4: required list<ClientAndNextHops> nextHopMulti,
  5: required bool isConnected,
  6: optional AdminDistance adminDistance,
}

struct ArpEntryThrift {
  1: string mac,
  2: i32 port,
  3: string vlanName,
  4: Address.BinaryAddress ip,
  5: i32 vlanID,
  6: string state,
  7: i32 ttl,
}

struct L2EntryThrift {
  1: string mac,
  2: i32 port,
  3: i32 vlanID,
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
  1: i64 bytes,
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
  1: i16 minimumLength
  2: i16 maximumLength
}

struct QueueCongestionDetection {
  1: optional LinearQueueCongestionDetection linear
}

struct QueueCongestionBehavior {
  1: bool earlyDrop
  2: bool ecn
}

struct ActiveQueueManagement {
  1: QueueCongestionDetection detection
  2: QueueCongestionBehavior behavior
}

struct PortQueueThrift {
  1: i32 id,
  2: string name = "",
  3: string mode,
  4: optional i32 weight,
  5: optional i32 reservedBytes,
  6: optional string scalingFactor,
  7: optional ActiveQueueManagement aqm,
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
  14: bool fecEnabled, // Forward Error Correction port setting
  15: bool txPause = false,
  16: bool rxPause = false,
  17: list<PortQueueThrift> portQueues = [],
}

struct NdpEntryThrift {
  1: Address.BinaryAddress ip,
  2: string mac,
  3: i32 port,
  4: string vlanName,
  5: i32 vlanID,
  6: string state,
  7: i32 ttl,
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
}

enum CaptureDirection {
  CAPTURE_ONLY_RX = 0,
  CAPTURE_ONLY_TX = 1,
  CAPTURE_TX_RX = 2
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
}

enum StdClientIds {
  BGPD = 0,
  STATIC_ROUTE = 1,
  INTERFACE_ROUTE = 2,
  LINKLOCAL_ROUTE = 3,
  NETLINK_LISTENER = 100,
  OPENR = 786,
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

  /*
   * Begins a packet stream from the switch to a distribution service
   */
  void beginPacketDump(1: i32 port)

  /*
   * Kill the pcap distribution process
   */
  void killDistributionProcess()

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
   * Return info related to the port including name, description, speed,
   * counters, ...
   */
  PortInfoThrift getPortInfo(1: i32 portId)
    throws (1: fboss.FbossBaseError error)
  map<i32, PortInfoThrift> getAllPortInfo()
    throws (1: fboss.FbossBaseError error)

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

  AggregatePortThrift getAggregatePort(1: i32 aggregatePortID)
    throws (1: fboss.FbossBaseError error)
  list<AggregatePortThrift> getAggregatePortTable()
    throws (1: fboss.FbossBaseError error)

  LacpPartnerPair getLacpPartnerPair(1: i32 portID)
    throws (1: fboss.FbossBaseError error)
  list<LacpPartnerPair> getAllLacpPartnerPairs()
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
   * Subscribe to a set of high-resolution counters
   */
  bool subscribeToCounters(1: highres.CounterSubscribeRequest req)
    throws (1: fboss.FbossBaseError error)

  /*
   * Log all updates to routes that match this prefix, or are more
   * specific.
   */
  void startLoggingRouteUpdates(1: RouteUpdateLoggingInfo info)
  void stopLoggingRouteUpdates(1: IpPrefix prefix, 2: string identifier)
  void stopLoggingAnyRouteUpdates(1: string identifier)
  list<RouteUpdateLoggingInfo> getRouteUpdateLoggingTrackedPrefixes()

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
