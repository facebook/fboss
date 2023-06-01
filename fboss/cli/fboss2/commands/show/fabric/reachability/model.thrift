namespace cpp2 facebook.fboss.cli

struct ShowFabricReachabilityModel {
  1: list<ReachabilityEntry> reachabilityEntries;
}

struct ReachabilityEntry {
  1: string switchName;
  2: list<string> reachablePorts;
}
