namespace cpp2 facebook.fboss.cli

struct ShowMirrorModel {
  1: list<ShowMirrorModelEntry> mirrorEntries;
}

struct ShowMirrorModelEntry {
  1: string mirror;
  2: string status;
  3: string egressPort;
  4: string egressPortName;
  5: string mirrorTunnelType;
  6: string srcMAC;
  7: string srcIP;
  8: string srcUDPPort;
  9: string dstMAC;
  10: string dstIP;
  11: string dstUDPPort;
  12: string dscp;
  13: string ttl;
}
