namespace cpp2 facebook.fboss.cli

struct ShowSystemPortModel {
  1: list<SystemPortEntry> sysPortEntries;
}

struct SystemPortEntry {
  1: i32 id;
  2: string name;
  3: i64 coreIndex;
  4: i64 corePortIndex;
  5: string speed;
  6: i64 numVoqs;
  7: string adminState;
  8: string qosPolicy;
  9: SystemPortHwStatsEntry hwPortStats;
}

struct SystemPortHwStatsEntry {
  1: map<i16, i64> egressDiscardBytes;
  2: map<i16, i64> egressOutBytes;
  3: map<i16, i64> egressWatermarkBytes;
}
