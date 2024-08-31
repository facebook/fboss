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
}
