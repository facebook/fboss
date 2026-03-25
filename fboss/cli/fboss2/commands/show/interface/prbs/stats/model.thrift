package "facebook.com/fboss/cli"

namespace cpp2 facebook.fboss.cli

include "fboss/lib/phy/phy.thrift"

struct ShowPrbsStatsModel {
  1: list<PrbsStatsInterfaceEntry> interfaceEntries;
}

struct PrbsStatsInterfaceEntry {
  1: list<PrbsStatsComponentEntry> componentEntries;
}

struct PrbsStatsComponentEntry {
  1: string interfaceName;
  2: phy.PortComponent component;
  3: phy.PrbsStats prbsStats;
}
