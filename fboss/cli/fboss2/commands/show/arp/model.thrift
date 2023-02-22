namespace cpp2 facebook.fboss.cli

struct ShowArpModel {
  1: list<ArpEntry> arpEntries;
}

struct ArpEntry {
  1: string ip;
  2: string mac;
  3: i32 port;
  4: string vlan;
  5: string state;
  6: i32 ttl;
  7: i32 classID;
  8: string ifName;
  9: string switchName;
}
