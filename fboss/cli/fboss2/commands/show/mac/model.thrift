namespace cpp2 facebook.fboss.cli

struct ShowMacAddrToBlockModel {
  1: list<MacAndVlan> macAndVlanEntries;
}

struct MacAndVlan {
  1: i32 vlanID;
  2: string macAddress;
}
