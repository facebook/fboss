namespace cpp2 facebook.fboss.cli

struct ShowFabricModel {
  1: list<FabricEntry> fabricEntries;
}

struct FabricEntry {
  1: string localPort;
  2: string remoteSystem;
}
