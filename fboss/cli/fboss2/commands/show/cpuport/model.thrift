package "facebook.com/fboss/cli"

namespace cpp2 facebook.fboss.cli

struct ShowCpuPortModel {
  1: list<CpuPortQueueEntry> cpuQueueEntries_DEPRECATED;
  2: map<i32, list<CpuPortQueueEntry>> cpuPortStatEntries;
}

struct CpuPortQueueEntry {
  1: i32 id;
  2: string name;
  3: i64 ingressPackets;
  4: i64 discardPackets;
}
