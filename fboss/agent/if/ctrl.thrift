namespace cpp facebook.fboss
namespace cpp2 facebook.fboss
namespace py neteng.fboss.ctrl
namespace d neteng.fboss.ctrl

include "fboss/agent/if/fboss.thrift"
include "common/fb303/if/fb303.thrift"
include "common/network/if/Address.thrift"
include "fboss/agent/if/optic.thrift"
include "fboss/agent/if/highres.thrift"

typedef binary (cpp2.type = "::folly::fbstring") fbbinary
typedef string (cpp2.type = "::folly::fbstring") fbstring

const i32 DEFAULT_CTRL_PORT = 5909

struct IpPrefix {
  1: required Address.BinaryAddress ip,
  2: required i16 prefixLength,
}

struct UnicastRoute {
  1: required IpPrefix dest,
  2: required list<Address.BinaryAddress> nextHopAddrs,
}

struct ArpEntryThrift {
  1: string mac,
  2: i32 port,
  3: string vlanName,
  4: Address.BinaryAddress ip,
  5: i32 vlanID,
}

struct InterfaceDetail {
  1: string interfaceName,
  2: i32 interfaceId,
  3: i32 vlanId,
  4: i32 routerId,
  5: string mac,
  6: list<IpPrefix> address,
}

struct ProductInfo {
  1: string oem,
  2: string product,
  3: string serial,
  4: string backplaneOem,
  5: string backplaneProduct,
  6: string backplaneSerial,
  7: string macBase,
  8: i16 cardIndex,
  9: string mgmtInterface,
}

struct PortErrors {
  1: i64 errors,
  2: i64 discards,
}

struct PortCounters {
  1: i64 bytes,
  2: i64 ucastPkts,
  3: i64 multicastPkts,
  4: i64 broadcastPkts,
  5: PortErrors errors,
}

enum PortAdminState {
  DISABLED = 0,
  ENABLED = 1,
}

enum PortOperState {
  DOWN = 0,
  UP = 1,
}

struct PortStatThrift {
  1: i32 portId,
  2: i64 speedMbps,
  3: PortAdminState adminState,
  4: PortOperState operState,

  10: PortCounters output,
  11: PortCounters input,
}

struct NdpEntryThrift {
  1: Address.BinaryAddress ip,
  2: string mac,
  3: i32 port,
  4: string vlanName,
  5: i32 vlanID,
}

enum BootType {
  UNINITIALIZED = 0,
  COLD_BOOT = 1,
  WARM_BOOT = 2
}

struct PortStatus {
  1: bool enabled,
  2: bool up
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
   * Send packets in binary or hex format to controller
   */
  void sendPkt(1: i32 port, 2: i32 vlan, 3: fbbinary data)
    throws (1: fboss.FbossBaseError error)
  void sendPktHex(1: i32 port, 2: i32 vlan, 3: fbstring hex)
    throws (1: fboss.FbossBaseError error)

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
   */
  UnicastRoute getIpRoute(1: Address.Address addr 2: i32 vrfId)
    throws (1: fboss.FbossBaseError error)
  map<i32, InterfaceDetail> getAllInterfaces()
    throws (1: fboss.FbossBaseError error)
  void registerForPortStatusChanged()
    throws (1: fboss.FbossBaseError error) (thread='eb')
  list<string> getInterfaceList()
    throws (1: fboss.FbossBaseError error)
  list<UnicastRoute> getRouteTable()
    throws (1: fboss.FbossBaseError error)
  map<i32, PortStatus> getPortStatus(1: list<i32> ports)
    throws (1: fboss.FbossBaseError error)
  InterfaceDetail getInterfaceDetail(1: i32 interfaceId)
    throws (1: fboss.FbossBaseError error)

  /*
   * Returns all interface related stats for given interfaceId
   */
  PortStatThrift getPortStats(1: i32 portId)
    throws (1: fboss.FbossBaseError error)
  map<i32, PortStatThrift> getAllPortStats()
    throws (1: fboss.FbossBaseError error)

  list<ArpEntryThrift> getArpTable()
    throws (1: fboss.FbossBaseError error)
  list<NdpEntryThrift> getNdpTable()
    throws (1: fboss.FbossBaseError error)
  /*
   * Returns all the DOM information
   */
  map<i32, optic.SfpDom> getSfpDomInfo(1: list<i32> port)
    throws (1: fboss.FbossBaseError error)

  /*
   * Return product information
   */
  ProductInfo getProductInfo()
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

  void keepalive()

  i32 getIdleTimeout()
    throws (1: fboss.FbossBaseError error)
}

service PortStatusListenerClient extends fb303.FacebookService {
  void portStatusChanged(1: i32 id, 2: PortStatus ps)
    throws (1: fboss.FbossBaseError error)
}
