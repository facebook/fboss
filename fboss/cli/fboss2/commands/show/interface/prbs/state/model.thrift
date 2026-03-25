package "facebook.com/fboss/cli"

namespace cpp2 facebook.fboss.cli

include "fboss/lib/phy/phy.thrift"
include "fboss/lib/phy/prbs.thrift"

struct ShowPrbsStateModel {
  1: list<PrbsStateInterfaceEntry> interfaceEntries;
}

struct PrbsStateInterfaceEntry {
  1: list<PrbsStateComponentEntry> componentEntries;
}

struct PrbsStateComponentEntry {
  1: string interfaceName;
  2: phy.PortComponent component;
  3: prbs.InterfacePrbsState prbsState;
}
