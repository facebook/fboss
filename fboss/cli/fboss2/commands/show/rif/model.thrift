namespace cpp2 facebook.fboss.cli

struct ShowRifModel {
  1: list<RifEntry> rifs;
}

struct RifEntry {
  1: string name;
  2: i32 rifID;
  3: optional i32 vlanID;
  4: i32 routerID;
  5: string mac;
  6: list<string> addrs;
  7: i32 mtu;
}
