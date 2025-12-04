package "facebook.com/fboss/cli"

namespace cpp2 facebook.fboss.cli

struct InterfaceFlapsModel {
  1: list<FlapCounters> flap_counters;
}

struct FlapCounters {
  1: string interfaceName;
  2: i64 oneMinute;
  3: i64 tenMinute;
  4: i64 oneHour;
  5: i64 totalFlaps;
}
