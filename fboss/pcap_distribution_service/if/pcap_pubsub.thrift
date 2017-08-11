namespace cpp2 facebook.fboss
namespace py neteng.fboss.pcap_pubsub
namespace py.asyncio neteng.fboss.asyncio.pcap_pubsub
namespace d neteng.fboss.pcap_pubsub

typedef binary (cpp2.type = "::folly::fbstring") fbbinary

const i32 PCAP_PUBSUB_PORT = 5911

// A struct to hold a Broadcom reason for
// why a packet was sent to the CPU
struct RxReason {
  // a integer encoding of the reason
  1: required i32 bytes,
  // a human readable description of the reason
  2: required string description
}

// A struct holding data of a packet received by the CPU
struct RxPacketData {
  1: required i32 srcPort,
  2: required i32 srcVlan,
  // The data in the packet
  3: required fbbinary packetData,
  // A list of the reasons that the packet
  // was sent to the CPU
  4: required list<RxReason> reasons
}

// A struct holding data of a packet that was sent out
// of the CPU
struct TxPacketData {
  1: required fbbinary packetData
}

union PacketData {
  1: RxPacketData rxpkt,
  2: TxPacketData txpkt
}

struct CapturedPacket {
  1: required bool rx,
  2: required PacketData pkt
}

// This interface is for a user to connect to the service,
// and open subscriptions and request packet dumps
service PcapPushSubscriber {
  // Called by the switch to send a packet to the
  // distributor
  void receiveRxPacket(1: RxPacketData packet, 2: i16 type)
  void receiveTxPacket(1: TxPacketData packet, 2: i16 type)

  // Give the switch the ability to kill the distribution
  // process if needed
  void kill()

  // Call by a client to subscribe or unsubscribe from
  // the service
  void subscribe(1: string client, 2: i32 port)
  void unsubscribe(1: string client, 2: i32 port)

  // Dump the requested packet as a list
  // Request by type of packet, or get all ethertypes
  list<CapturedPacket> dumpAllPackets()
  list<CapturedPacket> dumpPacketsByType(1: list<i16> ethertypes)
}

// This interface is for a subscriber to receive a packet stream
// from the service
service PcapSubscriber {
  // Both of these functions will be called by the
  // distributor upon receiving a packet from the switch.
  void receiveRxPacket(1: RxPacketData packet)
  void receiveTxPacket(1: TxPacketData packet)
}
