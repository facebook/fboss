namespace cpp2 facebook.fboss.cli

struct ShowAclModel {
  1: list<AclEntry> aclEntries;
}

struct AclEntry {
  1: string name;
  2: i32 priority;
  3: byte proto;
  4: i16 srcPort;
  5: i16 dstPort;
  6: byte ipFrag;
  7: byte dscp;
  8: byte ipType;
  9: byte icmpType;
  10: byte icmpCode;
  11: i16 ttl;
  12: i16 l4SrcPort;
  13: i16 l4DstPort;
  14: string dstMac;
  15: byte lookupClassL2;
  16: string actionType;
}
