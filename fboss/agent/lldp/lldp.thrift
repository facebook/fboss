namespace py neteng.fboss.lldp
namespace py3 neteng.fboss.lldp
namespace py.asyncio neteng.fboss.asyncio.lldp
namespace cpp2 facebook.fboss.lldp
namespace php facebook.fboss.lldp
namespace rust facebook.fboss.lldp
namespace go facebook.fboss.lldp

enum LinkProtocol {
  UNKNOWN = 1,
  LLDP = 2,
  CDP = 3,
}

enum LldpChassisIdType {
  RESERVED = 0,
  CHASSIS_ALIAS = 1,
  INTERFACE_ALIAS = 2,
  PORT_ALIAS = 3,
  MAC_ADDRESS = 4,
  NET_ADDRESS = 5,
  INTERFACE_NAME = 6,
  LOCALLY_ASSIGNED = 7,
}

enum LldpPortIdType {
  RESERVED = 0,
  INTERFACE_ALIAS = 1,
  PORT_ALIAS = 2,
  MAC_ADDRESS = 3,
  NET_ADDRESS = 4,
  INTERFACE_NAME = 5,
  AGENT_CIRCUIT_ID = 6,
  LOCALLY_ASSIGNED = 7,
}

struct LinkNeighborFields {
  1: LinkProtocol protocol = LinkProtocol.UNKNOWN;
  2: i16 localPort;
  3: i16 localVlan;
  4: string srcMac;

  5: LldpChassisIdType chassisIdType = LldpChassisIdType.RESERVED;
  6: LldpPortIdType portIdType = LldpPortIdType.RESERVED;

  // 16 bit unsigned, represented as 32 bit in thrift
  7: i32 capabilities;
  8: i32 enabledCapabilities;

  9: string chassisId;
  10: string portId;
  11: string systemName;
  12: string portDescription;
  13: string systemDescription;

  // seconds
  14: i64 receivedTTL;
  15: i64 expirationTime;
}
