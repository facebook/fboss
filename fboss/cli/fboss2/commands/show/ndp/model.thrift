namespace cpp2 facebook.fboss.cli

struct ShowNdpModel {
  1: list<NdpEntry> ndpEntries;
}

struct NdpEntry {
  1: string ip;
  2: string mac;
  3: string port;
  4: string vlanName;
  5: i32 vlanID;
  6: string state;
  7: i32 ttl;
  8: i32 classID;
  9: bool isLocal;
  10: optional i64 switchId;
}
