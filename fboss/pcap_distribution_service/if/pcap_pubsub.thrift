namespace cpp2 facebook.fboss
namespace py neteng.fboss.pcap_pubsub
namespace py.asyncio neteng.fboss.asyncio.pcap_pubsub
namespace d neteng.fboss.pcap_pubsub

typedef binary (cpp2.type = "::folly::fbstring") fbbinary


const i32 PCAP_PUBSUB_PORT = 5911

struct RxPacketData {
  1: required i32 srcPort,
  2: required i32 srcVlan,
  3: required fbbinary packetData
}

// can expand this packet later
struct TxPacketData {
  1: required fbbinary packetData
}

service PcapPushSubscriber {
  // Called by the switch to send a packet to the
  // distributor
  void receiveRxPacket(1: RxPacketData packet)
  void receiveTxPacket(1: TxPacketData packet)

  // Give the switch the ability to kill the distribution
  // process if needed
  void kill()

  // Call by a client to subscribe or unsubscribe from
  // the service
  void subscribe(1: string client, 2: i32 port)
  void unsubscribe(1: string client, 2: i32 port)
}

service PcapSubscriber {
  // Both of these functions will be called by the
  // distributor upon receiving a packet from the switch.
  void receiveRxPacket(1: RxPacketData packet)
  void receiveTxPacket(1: TxPacketData packet)
}
