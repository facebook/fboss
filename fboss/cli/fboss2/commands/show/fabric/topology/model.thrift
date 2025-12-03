package "facebook.com/fboss/cli"

namespace cpp2 facebook.fboss.cli

struct ShowFabricTopologyModel {
  1: list<FabricVirtualDeviceTopology> virtualDeviceTopology;
}

struct FabricVirtualDeviceTopology {
  1: i64 virtualDeviceId;
  2: i32 numConnections;
  3: i64 remoteSwitchId;
  4: string remoteSwitchName;
  5: list<string> connectingPorts;
  6: bool isSymmetric;
}
