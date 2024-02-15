namespace cpp2 facebook.fboss.cli

struct ShowInterfaceModel {
  1: list<Interface> interfaces;
}

struct Interface {
  1: string name;
  2: string description;
  3: string status;
  4: string speed;
  5: optional i32 vlan;
  6: optional i32 mtu;
  7: list<IpPrefix> prefixes;
}

struct IpPrefix {
  1: string ip;
  2: i16 prefixLength;
}
