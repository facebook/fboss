namespace cpp2 facebook.fboss.cli

struct ShowPortModel {
  1: list<PortEntry> portEntries;
}

struct PortEntry {
  1: i32 id;
  2: string name;
  3: string adminState;
  4: string linkState;
  5: string speed;
  6: string profileId;
  7: i32 tcvrID;
  8: string tcvrPresent;
  9: optional i32 hwLogicalPortId;
  10: i32 numUnicastQueues;
  11: optional string pause;
  12: optional string pfc;
  13: PortHwStatsEntry hwPortStats;
  14: map<i16, string> queueIdToName;
  15: string isDrained;
  16: string activeState;
  17: string activeErrors;
  18: string coreId;
  19: string virtualDeviceId;
}

struct PortHwStatsEntry {
  1: i64 inUnicastPkts;
  2: i64 inDiscardPkts;
  3: i64 inErrorPkts;
  4: i64 outDiscardPkts;
  5: i64 outCongestionDiscardPkts;
  6: map<i16, i64> queueOutDiscardBytes;
  7: map<i16, i64> queueOutBytes;
  8: optional map<i16, i64> outPfcPriorityPackets;
  9: optional map<i16, i64> inPfcPriorityPackets;
  10: optional i64 outPfcPackets;
  11: optional i64 inPfcPackets;
  12: optional i64 outPausePackets;
  13: optional i64 inPausePackets;
  14: i64 ingressBytes;
  15: i64 egressBytes;
  16: i64 inCongestionDiscards;
}
