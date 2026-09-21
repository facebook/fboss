package "facebook.com/fboss/cli"

namespace cpp2 facebook.fboss.cli

struct ShowHwAgentStatusModel {
  1: list<HwAgentStatusEntry> hwAgentStatusEntries;
}

struct HwAgentStatusEntry {
  1: i32 switchIndex;
  2: i32 switchId;
  3: string runState;
  4: i32 linkSyncActive;
  5: i32 linkActiveSyncActive;
  6: i32 statsSyncActive;
  7: i32 fdbSyncActive;
  8: i32 rxPktSyncActive;
  9: i32 txPktSyncActive;
  10: i32 switchReachabilityChangeSyncActive;
  11: i64 linkEventsSent;
  12: i64 linkEventsReceived;
  13: i64 txPktEventsReceived;
  14: i64 txPktEventsSent;
  15: i64 rxPktEventsSent;
  16: i64 rxPktEventsReceived;
  17: i64 fdbEventsSent;
  18: i64 fdbEventsReceived;
  19: i64 switchReachabilityChangeEventsSent;
  20: i64 switchReachabilityChangeEventsReceived;
  21: i64 HwSwitchStatsEventsSent;
  22: i64 HwSwitchStatsEventsReceived;
}
