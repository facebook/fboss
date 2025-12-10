package "facebook.com/fboss/cli"

namespace cpp2 facebook.fboss.cli

struct ShowMacAddrToBlockModel {
  1: list<MacAndVlan> macAndVlanEntries;
}

struct MacAndVlan {
  1: i32 vlanID;
  2: string macAddress;
}

struct ShowMacDetailsModel {
  1: list<L2Entry> l2Entries;
}

struct L2Entry {
  1: string mac;
  2: i32 port;
  3: i32 vlanID;
  4: string l2EntryType;
  5: string ifName;
  6: string classID;
  7: optional i32 trunk;
}
