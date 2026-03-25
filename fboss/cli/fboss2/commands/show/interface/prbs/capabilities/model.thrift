package "facebook.com/fboss/cli"

namespace cpp2 facebook.fboss.cli

include "fboss/lib/phy/phy.thrift"
include "fboss/lib/phy/prbs.thrift"

struct ShowCapabilitiesModel {
  1: list<InterfaceEntry> interfaceEntries;
}

struct InterfaceEntry {
  1: list<ComponentEntry> componentEntries;
}

struct ComponentEntry {
  1: string interfaceName;
  2: phy.PortComponent component;
  3: list<prbs.PrbsPolynomial> prbsCapabilties;
}
