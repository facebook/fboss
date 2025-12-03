package "facebook.com/fboss/cli"

namespace cpp2 facebook.fboss.cli

struct ShowAclModel {
  1: map<string, list<AclEntry>> aclTableEntries;
}

struct AclEntry {
  1: string name;
  2: i32 priority;
  3: i16 proto;
  4: i16 srcPort;
  5: i16 dstPort;
  6: i16 ipFrag;
  7: i16 dscp;
  8: i16 ipType;
  9: i16 icmpType;
  10: i16 icmpCode;
  11: i16 ttl;
  12: i16 l4SrcPort;
  13: i16 l4DstPort;
  14: string dstMac;
  15: i16 lookupClassL2;
  16: string actionType;
  17: bool enabled;
}
