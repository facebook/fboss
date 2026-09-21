package "facebook.com/fboss/cli"

namespace cpp2 facebook.fboss.cli

struct InterfaceCountersModel {
  1: list<InterfaceCounters> int_counters;
}

struct InterfaceCounters {
  1: string interfaceName;
  2: i64 inputBytes;
  3: i64 inputUcastPkts;
  4: i64 inputMulticastPkts;
  5: i64 inputBroadcastPkts;
  6: i64 outputBytes;
  7: i64 outputUcastPkts;
  8: i64 outputMulticastPkts;
  9: i64 outputBroadcastPkts;
}
