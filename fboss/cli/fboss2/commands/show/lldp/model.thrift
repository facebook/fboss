namespace cpp2 facebook.fboss.cli

struct ShowLldpModel {
  1: list<LldpEntry> lldpEntries;
}

struct LldpEntry {
  1: string localPort;
  2: string systemName;
  3: string remotePort;
}
