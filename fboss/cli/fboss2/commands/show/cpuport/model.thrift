namespace cpp2 facebook.fboss.cli

struct ShowCpuPortModel {
  1: list<CpuPortQueueEntry> cpuQueueEntries;
}

struct CpuPortQueueEntry {
  1: i32 id;
  2: string name;
  3: i64 ingressPackets;
  4: i64 discardPackets;
}
