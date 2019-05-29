namespace cpp2 facebook.fboss
namespace py neteng.fboss.sflow
namespace py3 neteng.fboss

// teimstamp encoding for packet capture time
struct SflowTimestamp {
  1: i64 seconds,
  2: i64 nanoseconds
}

//
// Metadata and body of the captured packet
//
struct SflowPacketInfo {
  1: SflowTimestamp timestamp,
  2: bool ingressSampled,
  3: bool egressSampled,

  // Physical port information
  4: i16 srcPort, //int16
  5: i16 dstPort, //uint16

  // VLAN information since it may not be in the packet
  6: i16 vlan, //uint16

  // The packet itself
  7: binary packetData

  // Frame length
  8: i32 frameLength,

  // Payload removed
  9: i32 payloadRemoved
}
