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
  2: phy.PrbsComponent component;
  3: phy.PrbsStats prbsStats;
}
